#include <SDL.h>

#include "Xmi.h"
#include "MusicDevice.h"
#include "Prefs.h"

unsigned int NumTracks;
char ChannelThread[16]; // 16 device channels
int NumUsedChannels;    // number of in-use device channels

MIDI_EVENT **TrackEvents;
short *TrackTiming;
unsigned short *TrackUsedChannels;
TrackLoop *TrackLoops;

char ThreadChannelRemap[16 * NUM_THREADS];
SDL_atomic_t DeviceChannelVolume[16]; // only msb: 0-127
SDL_atomic_t ThreadPlaying[NUM_THREADS];
SDL_atomic_t ThreadCommand[NUM_THREADS];

MusicDevice *MusicDev;
static SDL_mutex *MyMutex;

void MusicCallback(void *userdata, Uint8 *stream, int len) {
    MusicDevice *dev;

    SDL_LockMutex(MyMutex);
    dev = *(MusicDevice **)userdata;
    if (!dev || !dev->isOpen) {
        SDL_UnlockMutex(MyMutex);
        return;
    }

    SDL_memset(stream, 0, (size_t)len); // in case we don't get anything
    dev->generate(dev, (short *)((void *)stream), len / (int)(2 * sizeof(short)));
    SDL_UnlockMutex(MyMutex);
}

void FreeXMI(void) {
    unsigned int track;
    MIDI_EVENT *event, *next;

    for (track = 0; track < NumTracks; track++) {
        event = TrackEvents[track];
        while (event) {
            next = event->next;
            if (event->buffer)
                free(event->buffer);
            free(event);
            event = next;
        }
    }

    if (TrackEvents) {
        free(TrackEvents);
        TrackEvents = 0;
    }
    if (TrackTiming) {
        free(TrackTiming);
        TrackTiming = 0;
    }
    if (TrackUsedChannels) {
        free(TrackUsedChannels);
        TrackUsedChannels = 0;
    }
    if (TrackLoops) {
        free(TrackLoops);
        TrackLoops = 0;
    }

    NumTracks = 0;
}

MIDI_EVENT *NewMIDIEvent(MIDI_EVENT **eventlist, MIDI_EVENT *curevent, int time) {
    if (*eventlist == 0) {
        *eventlist = curevent = (MIDI_EVENT *)malloc(sizeof(MIDI_EVENT));

        curevent->next = 0;

        if (time < 0)
            curevent->time = 0;
        else
            curevent->time = time;
        curevent->buffer = 0;
        curevent->len = 0;

        return curevent;
    }

    if (time < 0) {
        MIDI_EVENT *event = (MIDI_EVENT *)malloc(sizeof(MIDI_EVENT));

        event->next = *eventlist;
        *eventlist = curevent = event;

        curevent->time = 0;
        curevent->buffer = 0;
        curevent->len = 0;

        return curevent;
    }

    if (curevent->time > time)
        curevent = *eventlist;

    while (curevent->next) {
        if (curevent->next->time > time) {
            MIDI_EVENT *event = (MIDI_EVENT *)malloc(sizeof(MIDI_EVENT));

            event->next = curevent->next;
            curevent->next = event;
            curevent = event;

            curevent->time = time;
            curevent->buffer = 0;
            curevent->len = 0;

            return curevent;
        }

        curevent = curevent->next;
    }

    curevent->next = (MIDI_EVENT *)malloc(sizeof(MIDI_EVENT));

    curevent = curevent->next;
    curevent->next = 0;

    curevent->time = time;
    curevent->buffer = 0;
    curevent->len = 0;

    return curevent;
}

int ReadXMI(const char *filename) {
    FILE *f;
    int size, start, begin, pos, time, end, tempo, tempo_set;
    unsigned int i, count, len, chunk_len, quant, status, delta, b0, b1, b2, b3;
    unsigned char *data, *p;
    short ppqn;
    unsigned short used_channels;
    MIDI_EVENT *eventlist, *curevent, *prev;
    MIDI_EVENT *loop_start, *loop_end;
    int loop_count;
    char buf[32];
    MusicMode mode = Music_GeneralMidi;

    INFO("Reading XMI %s", filename);

    extern FILE *fopen_caseless(const char *path, const char *mode); // see caseless.c
    f = fopen_caseless(filename, "rb");
    if (f == 0) {
        ERROR("Could not read XMI");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (unsigned char *)malloc(size);
    if (fread(data, size, 1, f) != 1) {
        free(data);
        fclose(f);
        return 0;
    }
    fclose(f);

    p = data;

    memcpy(buf, p, 4);
    p += 4;
    if (memcmp(buf, "FORM", 4)) {
        free(data);
        return 0;
    } // is not an xmi

    b3 = *p++;
    b2 = *p++;
    b1 = *p++;
    b0 = *p++;
    len = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

    start = p - data;

    memcpy(buf, p, 4);
    p += 4;
    if (!memcmp(buf, "XMID", 4))
        NumTracks = 1; // XMI doesn't have XDIR, so there's only one track
    else if (memcmp(buf, "XDIR", 4)) {
        free(data);
        return 0;
    } // invalid XMI format
    else {
        NumTracks = 0;

        for (i = 4; i < len; i++) {
            memcpy(buf, p, 4);
            p += 4;

            b3 = *p++;
            b2 = *p++;
            b1 = *p++;
            b0 = *p++;
            chunk_len = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

            i += 8;

            if (memcmp(buf, "INFO", 4)) {
                p += ((chunk_len + 1) & ~1);
                i += ((chunk_len + 1) & ~1);
                continue;
            }

            if (chunk_len < 2)
                break;

            b0 = *p++;
            b1 = *p++;
            NumTracks = b0 | (b1 << 8);

            break;
        }

        if (NumTracks == 0) {
            free(data);
            return 0;
        } // xmi must have at least one track

        p = data + start + ((len + 1) & ~1);

        memcpy(buf, p, 4);
        p += 4;
        if (memcmp(buf, "CAT ", 4)) {
            free(data);
            return 0;
        } // invalid XMI format

        b3 = *p++;
        b2 = *p++;
        b1 = *p++;
        b0 = *p++;
        len = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

        memcpy(buf, p, 4);
        p += 4;
        if (memcmp(buf, "XMID", 4)) {
            free(data);
            return 0;
        } // invalid XMI format
    }

    INFO("NumTracks: %i", NumTracks);

    TrackEvents = (MIDI_EVENT **)malloc(NumTracks * sizeof(MIDI_EVENT *));
    TrackTiming = (short *)malloc(NumTracks * sizeof(short));
    TrackUsedChannels = (unsigned short *)malloc(NumTracks * sizeof(unsigned short));
    TrackLoops = (TrackLoop *)calloc(NumTracks, sizeof(TrackLoop));

    for (i = 0; i < NumTracks; i++)
        TrackEvents[i] = 0;

    count = 0;

    while (p - data < size && count != NumTracks) {
        memcpy(buf, p, 4);
        p += 4;

        b3 = *p++;
        b2 = *p++;
        b1 = *p++;
        b0 = *p++;
        len = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

        if (!memcmp(buf, "FORM", 4)) {
            p += 4;

            memcpy(buf, p, 4);
            p += 4;

            b3 = *p++;
            b2 = *p++;
            b1 = *p++;
            b0 = *p++;
            len = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
        }

        if (memcmp(buf, "EVNT", 4)) {
            p += ((len + 1) & ~1);
            continue;
        }

        eventlist = 0;
        curevent = 0;
        time = 0;
        end = 0;
        tempo = 500000;
        tempo_set = 0;
        status = 0;
        used_channels = 0;
        loop_start = loop_end = 0;
        loop_count = 0;

        begin = p - data;

        while (!end && p - data < size) {
            quant = 0;
            for (i = 0; i < 4; i++) {
                b0 = *p++;
                if (b0 & 0x80) {
                    p--;
                    break;
                }
                quant += b0;
            }
            time += quant * 3;

            status = *p++;
            switch (status >> 4) {
            case 0x9: // note on
                used_channels |= (1 << (status & 15));
                b0 = *p++;
                curevent = NewMIDIEvent(&eventlist, curevent, time);
                curevent->status = status;
                curevent->data[0] = b0;
                curevent->data[1] = *p++;
                delta = 0;
                for (i = 0; i < 4; i++) {
                    b1 = *p++;
                    delta <<= 7;
                    delta |= b1 & 0x7F;
                    if (!(b1 & 0x80))
                        break;
                }
                prev = curevent;
                curevent = NewMIDIEvent(&eventlist, curevent, time + delta * 3);
                curevent->status = status;
                curevent->data[0] = b0;
                curevent->data[1] = 0;
                curevent = prev;
                break;

            case 0x8:
            case 0xA:
            case 0xB:
            case 0xE: // note off, aftertouch, controller, pitch wheel
                used_channels |= (1 << (status & 15));
                curevent = NewMIDIEvent(&eventlist, curevent, time);
                curevent->status = status;
                curevent->data[0] = *p++;
                curevent->data[1] = *p++;
                if ((status >> 4) == 0xB) {
                    // XMIDI "FOR" loop start (116, data = repeat count, 0 = forever) and loop end (117)
                    if (curevent->data[0] == 116 && !loop_start) {
                        loop_start = curevent;
                        loop_count = curevent->data[1];
                    } else if (curevent->data[0] == 117 && loop_start && !loop_end) {
                        loop_end = curevent;
                    }
                }
                break;

            case 0xC:
            case 0xD: // program change, pressure
                used_channels |= (1 << (status & 15));
                curevent = NewMIDIEvent(&eventlist, curevent, time);
                curevent->status = status;
                curevent->data[0] = *p++;
                break;

            case 0xF: // sysex
                if (status == 0xFF) {
                    pos = p - data;
                    b0 = *p++;
                    if (b0 == 0x2F)
                        end = 1;
                    else if (b0 == 0x51 && !tempo_set) {
                        p++;
                        b3 = *p++;
                        b2 = *p++;
                        b1 = *p++;
                        tempo = (b1 | (b2 << 8) | (b3 << 16)) * 3;
                        tempo_set = 1;
                    } else if (b0 == 0x51 && tempo_set) {
                        quant = 0;
                        for (i = 0; i < 4; i++) {
                            b1 = *p++;
                            quant <<= 7;
                            quant |= b1 & 0x7F;
                            if (!(b1 & 0x80))
                                break;
                        }
                        p += quant;
                        break;
                    }
                    p = data + pos;
                }
                curevent = NewMIDIEvent(&eventlist, curevent, time);
                curevent->status = status;
                if (status == 0xFF)
                    curevent->data[0] = *p++;
                quant = 0;
                for (i = 0; i < 4; i++) {
                    b0 = *p++;
                    quant <<= 7;
                    quant |= b0 & 0x7F;
                    if (!(b0 & 0x80))
                        break;
                }
                curevent->len = quant;
                if (curevent->len) {
                    curevent->buffer = (unsigned char *)malloc(curevent->len);
                    memcpy(curevent->buffer, p, curevent->len);
                    p += curevent->len;
                }
                break;

            default:
                break;
            }
        }

        ppqn = (tempo * 3) / 25000;
        if (!ppqn)
            break; // unable to convert data

        TrackEvents[count] = eventlist;
        TrackTiming[count] = ppqn;
        TrackUsedChannels[count] = used_channels;
        if (loop_start && loop_end && loop_end->time > loop_start->time) {
            TrackLoops[count].start_event = loop_start;
            TrackLoops[count].end_event = loop_end;
            TrackLoops[count].count = loop_count;
        }
        count++;

        p = data + begin + ((len + 1) & ~1);
    }

    if (count != NumTracks) // failed to extract all tracks from XMI
    {
        free(data);
        NumTracks = count;
        FreeXMI();
        return 0;
    }

    free(data);

    // Setup a sound bank for res/sound/sblaster, or res/sound/genmidi
    if (strstr(filename, "sblaster") != NULL)
        mode = Music_SoundBlaster;
    SDL_LockMutex(MyMutex);
    if (MusicDev) {
        MusicDev->setupMode(MusicDev, mode);
    }
    SDL_UnlockMutex(MyMutex);
    switch (mode) {
    case Music_GeneralMidi:
        INFO("Set General MIDI mode");
        break;
    case Music_SoundBlaster:
        INFO("Set Sound Blaster mode");
        break;
    }

    return 1; // success
}

//------------------------------------------------------------------------------
//  Sequencer
//
//  Event times are in units of 1/360 s: XMI delays are 1/120 s (independent of the
//  tempo meta event, which is only informational) and ReadXMI multiplies them by 3.
//
//  Like Miles/AIL, tracks can loop themselves between the XMIDI "FOR" (116) and
//  "NEXT" (117) controllers, and a "master" track (track 0 of a theme) fires
//  callback-trigger (119) controllers once per bar. Theme playback is driven by
//  those triggers (see XmiSetRequests below), as it was in the DOS version.
//------------------------------------------------------------------------------

#define TICK_US (1000000.0 / 360.0)
#define MAX_PENDING 128
#define MAX_PIECES (NUM_THREADS - 1) // threads 0..MAX_PIECES-1 play pieces, the last one is the master clock

// a note-off belonging to a previous pass of a loop, still to be sent
typedef struct {
    int time;
    MIDI_EVENT *ev;
} PendingOff;

typedef struct {
    MIDI_EVENT *event; // next event of the track, NULL when exhausted
    double start_us;   // wall clock time (us) that event time 0 corresponds to
    double last_tick;  // time (incl. loop offset) of the last event processed
    double last_time;  // when that event was scheduled, in us since start_us
    int offset;        // added to event times on the current pass of a loop
    int is_master;
    int looping;
    int loop_remaining; // passes still to jump back for, -1 = forever
    int loop_length;
    MIDI_EVENT *loop_start;
    MIDI_EVENT *loop_end;
    int npending;
    PendingOff pending[MAX_PENDING]; // sorted by time

    // volume of the whole track, 0-1000, which can be ramped up and down (AIL's sequence volume)
    volatile int vol1000;
    int vol_sent; // value last sent to the device
    int ramp_from, ramp_to;
    double ramp_start_us, ramp_len_us; // ramp_len_us == 0: not ramping
    int faded;                         // has been asked to fade out
    unsigned int track;
    unsigned short muted;              // track channels whose notes are being held back (crossfade)
} SeqThread;

static SeqThread Seq[NUM_THREADS];
static int ThreadReqTrack[NUM_THREADS];
static int ThreadReqLoop[NUM_THREADS];

// theme driving state
static int SlotPiece[NUM_THREADS];         // piece being played by each thread, -1 = none (sequencer thread only)
static XmiRequest Requests[MAX_PIECES];    // pieces the music AI wants playing, piece -1 = none
static SDL_mutex *RequestMutex;
static SDL_atomic_t AiDue;                 // set at the master's "run the AI" trigger
static unsigned char PieceMeasures[256];   // bars per piece, from thmN.dat
static int MasterMeasures = 1;             // bars per loop of the master track

extern volatile unsigned long mlimbs_counter; // bars since the AI last wrapped it; see mlimbs_do_ai

static double now_us(void) { return (double)SDL_GetPerformanceCounter() * 1000000.0 / (double)SDL_GetPerformanceFrequency(); }

static void dispatch_event(int i, const MIDI_EVENT *ev) {
    int channel = ev->status & 15;
    uint8_t p1 = ev->data[0];
    uint8_t p2 = ev->data[1];

    if (ev->status < 0x80 || ev->status >= 0xF0)
        return;

    // XMIDI-specific controllers (loops, callbacks, locks, bank/branch select) are not for the synth.
    // Note that 120 is "branch index" here and not General MIDI's "all sound off".
    if ((ev->status & ~15) == 0xB0 && p1 >= 110 && p1 <= 120)
        return;

    if ((ev->status & ~15) == 0x90 && p2) {
        // note on: nothing from channels that are crossfaded out; the drums have no channel of
        // their own, so the volume of the track scales their velocity instead
        if (channel != 9 && (Seq[i].muted & (1 << channel)))
            return;
        if (channel == 9 && Seq[i].vol1000 < 1000) {
            p2 = (p2 * Seq[i].vol1000) / 1000;
            if (!p2)
                return;
        }
    }

    if (channel != 9)
        channel = ThreadChannelRemap[channel + 16 * i]; // remap channel, except 9 (percussion)

    if ((ev->status & ~15) == 0xB0 && p1 == 0x07) {
        // set volume msb
        // store in array in case global volume changes later
        SDL_AtomicSet(&DeviceChannelVolume[channel], p2); // 0-127
        // send volume change to device
        SDL_LockMutex(MyMutex);
        if (MusicDev && MusicDev->isOpen) {
            // scale new volume according to global music volume and the volume of the track
            extern uchar curr_vol_lev; // 0-100
            int scaledVolume = ((int)p2 * (int)curr_vol_lev) / 100;
            if (channel != 9)
                scaledVolume = (scaledVolume * Seq[i].vol1000) / 1000;
            MusicDev->sendControllerChange(MusicDev, channel, p1, scaledVolume);
        }
        SDL_UnlockMutex(MyMutex);
        return;
    }

    SDL_LockMutex(MyMutex);
    if (MusicDev && MusicDev->isOpen) {
        switch (ev->status & ~15) {
        case 0x80:
            MusicDev->sendNoteOff(MusicDev, channel, p1, p2);
            break;
        case 0x90:
            MusicDev->sendNoteOn(MusicDev, channel, p1, p2);
            break;
        case 0xA0:
            MusicDev->sendNoteAfterTouch(MusicDev, channel, p1, p2);
            break;
        case 0xB0:
            MusicDev->sendControllerChange(MusicDev, channel, p1, p2);
            break;
        case 0xC0:
            MusicDev->sendProgramChange(MusicDev, channel, p1);
            break;
        case 0xD0:
            MusicDev->sendChannelAfterTouch(MusicDev, channel, p1);
            break;
        case 0xE0:
            MusicDev->sendPitchBendML(MusicDev, channel, p2, p1);
            break;
        }
    }
    SDL_UnlockMutex(MyMutex);
}

static void add_pending_off(SeqThread *t, int time, MIDI_EVENT *ev) {
    int n = t->npending, k;

    if (n >= MAX_PENDING)
        return; // can't happen with this data; losing a note-off is the lesser evil
    for (k = n; k > 0 && t->pending[k - 1].time > time; k--)
        t->pending[k] = t->pending[k - 1];
    t->pending[k].time = time;
    t->pending[k].ev = ev;
    t->npending = n + 1;
}

// Bring the channels of a track to a known state, since device channels get reused by different tracks.
static void reset_thread_channels(int thread) {
    extern uchar curr_vol_lev; // 0-100
    int channel;

    SDL_LockMutex(MyMutex);
    if (MusicDev && MusicDev->isOpen) {
        for (channel = 0; channel < 16; channel++) {
            if (channel == 9 || ChannelThread[channel] != thread)
                continue;
            MusicDev->sendControllerChange(MusicDev, channel, 123, 0); // all notes off
            MusicDev->sendControllerChange(MusicDev, channel, 64, 0);  // sustain
            MusicDev->sendControllerChange(MusicDev, channel, 1, 0);   // modulation
            MusicDev->sendControllerChange(MusicDev, channel, 10, 64); // pan
            MusicDev->sendControllerChange(MusicDev, channel, 11, 127); // expression
            MusicDev->sendControllerChange(MusicDev, channel, 7,
                                           (((127 * (int)curr_vol_lev) / 100) * Seq[thread].vol1000) / 1000);
            MusicDev->sendPitchBendML(MusicDev, channel, 64, 0);
        }
    }
    SDL_UnlockMutex(MyMutex);
}

// Send the volume of every channel of a track to the device again, after the track's volume has changed.
static void refresh_thread_volume(int thread) {
    extern uchar curr_vol_lev; // 0-100
    SeqThread *t = &Seq[thread];
    int channel;

    SDL_LockMutex(MyMutex);
    if (MusicDev && MusicDev->isOpen) {
        for (channel = 0; channel < 16; channel++) {
            if (channel == 9 || ChannelThread[channel] != thread)
                continue;
            MusicDev->sendControllerChange(
                MusicDev, channel, 7,
                (((int)SDL_AtomicGet(&DeviceChannelVolume[channel]) * (int)curr_vol_lev) / 100 * t->vol1000) / 1000);
        }
    }
    SDL_UnlockMutex(MyMutex);
    t->vol_sent = t->vol1000;
}

// Fade the volume of a track to to1000 (0-1000) over ms.
static void start_ramp(int thread, int to1000, int ms) {
    SeqThread *t = &Seq[thread];

    if (ms <= 0) {
        t->ramp_len_us = 0;
        t->vol1000 = to1000;
        refresh_thread_volume(thread);
        return;
    }
    t->ramp_from = t->vol1000;
    t->ramp_to = to1000;
    t->ramp_start_us = now_us();
    t->ramp_len_us = ms * 1000.0;
}

// Keep a ramp going. Called all the time; only talks to the device when the volume moved noticeably.
static void update_ramp(int thread, double now) {
    SeqThread *t = &Seq[thread];
    double f;
    int v;

    if (t->ramp_len_us <= 0)
        return;
    f = (now - t->ramp_start_us) / t->ramp_len_us;
    if (f >= 1.0) {
        f = 1.0;
        t->ramp_len_us = 0;
    }
    v = t->ramp_from + (int)((t->ramp_to - t->ramp_from) * f);
    if (t->ramp_len_us == 0 || abs(v - t->vol_sent) >= 8) {
        t->vol1000 = v;
        refresh_thread_volume(thread);
    }
}

// Hold back (or let through again) the notes of one channel of a track.
static void set_channel_muted(int thread, int track_channel, int muted) {
    SeqThread *t = &Seq[thread];

    if (muted) {
        t->muted |= (1 << track_channel);
        SDL_LockMutex(MyMutex);
        if (MusicDev && MusicDev->isOpen)
            MusicDev->sendControllerChange(MusicDev, ThreadChannelRemap[16 * thread + track_channel], 123, 0);
        SDL_UnlockMutex(MyMutex);
    } else {
        t->muted &= ~(1 << track_channel);
    }
}

// The channels of a piece that the crossfade works on: MIDI channels 11-16, as in the original.
#define XFADE_CHANNELS 0xFC00

// Stop a thread's track: silence and free its channels. Sequencer thread only.
static void seq_stop(int thread) {
    SeqThread *t = &Seq[thread];
    int channel;

    if (SDL_AtomicGet(&ThreadPlaying[thread])) {
        SDL_LockMutex(MyMutex);
        if (MusicDev && MusicDev->isOpen) {
            for (channel = 0; channel < 16; channel++) {
                if (channel != 9 && ChannelThread[channel] == thread) {
                    MusicDev->sendControllerChange(MusicDev, channel, 123, 0); // all notes off
                    MusicDev->sendControllerChange(MusicDev, channel, 64, 0);  // sustain off
                }
            }
        }
        SDL_UnlockMutex(MyMutex);
    }

    t->event = 0;
    t->npending = 0;
    t->looping = 0;
    t->ramp_len_us = 0;

    for (channel = 0; channel < 16; channel++) {
        if (ChannelThread[channel] == thread) {
            ChannelThread[channel] = -1;
            NumUsedChannels--;
            SDL_AtomicSet(&DeviceChannelVolume[channel], 0);
        }
    }

    SlotPiece[thread] = -1;
    SDL_AtomicSet(&ThreadPlaying[thread], 0);
}

// Start a track on a thread. Sequencer thread only. Returns 0 when the track couldn't be started.
static int seq_start(int thread, unsigned int track, int loop, double start_us, int vol1000, unsigned short muted) {
    SeqThread *t = &Seq[thread];
    char channel_remap[16];
    int num, trackChannel, deviceChannel;

    if (track >= NumTracks)
        return 0;

    if (SDL_AtomicGet(&ThreadPlaying[thread]))
        seq_stop(thread);

    num = GetTrackNumChannels(track);

    // check if enough device channels free; 16 channels available except one (percussion)
    if (NumUsedChannels + num > 16 - 1) {
        WARN("Not enough free MIDI channels to start track %u", track);
        return 0;
    }
    NumUsedChannels += num;

    memset(channel_remap, 0, 16);

    // assign channels used by track to device channels that are currently free
    for (trackChannel = 0; trackChannel < 16; trackChannel++) {
        // only map used, non-percussion channels
        if (trackChannel != 9 && (TrackUsedChannels[track] & (1 << trackChannel))) {
            // find first unassigned device channel
            for (deviceChannel = 0; deviceChannel < 16; deviceChannel++) {
                if (deviceChannel != 9 && ChannelThread[deviceChannel] == -1)
                    break;
            }
            channel_remap[trackChannel] = deviceChannel;
            ChannelThread[deviceChannel] = thread;
            // default to full volume
            SDL_AtomicSet(&DeviceChannelVolume[deviceChannel], 127);
        }
    }
    memcpy(ThreadChannelRemap + 16 * thread, channel_remap, 16);

    memset(t, 0, sizeof(*t));
    t->vol1000 = vol1000;
    t->track = track;
    t->muted = muted;
    reset_thread_channels(thread);
    t->event = TrackEvents[track];
    t->start_us = start_us;
    t->is_master = (thread == MASTER_THREAD);
    if (loop && TrackLoops && TrackLoops[track].start_event) {
        t->looping = 1;
        t->loop_start = TrackLoops[track].start_event;
        t->loop_end = TrackLoops[track].end_event;
        t->loop_length = t->loop_end->time - t->loop_start->time;
        t->loop_remaining = TrackLoops[track].count ? TrackLoops[track].count - 1 : -1;
    }

    SDL_AtomicSet(&ThreadPlaying[thread], 1);
    return 1;
}

// Called on the master's "start of bar" trigger: stop the pieces nobody wants any more, fade and
// crossfade the ones that are asked to, and start the requested ones that are due. A piece N bars long
// only starts on bars that are a multiple of N, and once started it loops by itself, so it stays in
// sync with the master.
// event_us is when the trigger was scheduled, which is when new pieces should start.
static void apply_requests(double event_us) {
    XmiRequest wanted[MAX_PIECES];
    int counter = (int)mlimbs_counter;
    int s, r, k;

    SDL_LockMutex(RequestMutex);
    memcpy(wanted, Requests, sizeof(wanted));
    SDL_UnlockMutex(RequestMutex);

    for (s = 0; s < MAX_PIECES; s++) {
        SeqThread *t = &Seq[s];
        unsigned short channels;
        int c;

        if (SlotPiece[s] < 0)
            continue;
        if (!SDL_AtomicGet(&ThreadPlaying[s])) {
            SlotPiece[s] = -1;
            continue;
        }
        for (r = 0; r < MAX_PIECES; r++)
            if (wanted[r].piece == SlotPiece[s])
                break;
        if (r == MAX_PIECES) {
            DEBUG("bar %d: stop piece %d", counter, SlotPiece[s]);
            seq_stop(s);
            continue;
        }

        // the piece stays, but may be asked to fade out
        if (wanted[r].ramp < 0 && !t->faded) {
            DEBUG("bar %d: fade out piece %d over %d ms", counter, SlotPiece[s], wanted[r].ramp_ms);
            t->faded = 1;
            start_ramp(s, 0, wanted[r].ramp_ms);
        }

        // ...or to bring in (lowest channel first) or take out (highest first) one more channel
        channels = TrackUsedChannels[t->track] & XFADE_CHANNELS;
        if (wanted[r].crossfade > 0 && (t->muted & channels)) {
            for (c = 10; c < 16; c++)
                if (t->muted & channels & (1 << c))
                    break;
            set_channel_muted(s, c, 0);
        } else if (wanted[r].crossfade < 0 && (channels & ~t->muted)) {
            for (c = 15; c >= 10; c--)
                if (channels & ~t->muted & (1 << c))
                    break;
            set_channel_muted(s, c, 1);
        }
    }

    for (r = 0; r < MAX_PIECES; r++) {
        int piece = wanted[r].piece;
        unsigned int track = piece + 1; // track 0 is the master
        int measures, vol1000;

        if (piece < 0 || piece > 255 || track >= NumTracks || !TrackUsedChannels[track])
            continue;

        for (k = 0; k < MAX_PIECES; k++)
            if (SlotPiece[k] == piece)
                break;
        if (k < MAX_PIECES)
            continue; // already playing

        measures = PieceMeasures[piece] ? PieceMeasures[piece] : 1;
        if ((counter * MasterMeasures) % measures != 0)
            continue;

        for (s = 0; s < MAX_PIECES; s++)
            if (SlotPiece[s] < 0 && !SDL_AtomicGet(&ThreadPlaying[s]))
                break;
        if (s == MAX_PIECES)
            continue; // no free thread

        // A piece that comes in while the theme is fading in or out starts silent. If it is crossfaded
        // in it starts with just the drums.
        vol1000 = wanted[r].volume * 10;
        if (seq_start(s, track, 1, event_us, wanted[r].ramp ? 0 : vol1000,
                      wanted[r].crossfade > 0 ? (TrackUsedChannels[track] & XFADE_CHANNELS) : 0)) {
            SlotPiece[s] = piece;
            if (wanted[r].ramp < 0)
                Seq[s].faded = 1;
            else if (wanted[r].ramp > 0)
                start_ramp(s, vol1000, wanted[r].ramp_ms);
            DEBUG("bar %d: start piece %d on thread %d%s", counter, piece, s,
                  wanted[r].ramp > 0 ? " (fading in)" : (wanted[r].ramp < 0 ? " (silent)" : ""));
        }
    }

    mlimbs_counter++;
}

// Process one event of a thread. Sequencer thread only.
static void process_event(int i, MIDI_EVENT *ev, int is_pending, double aim) {
    SeqThread *t = &Seq[i];

    if (is_pending) {
        dispatch_event(i, ev);
        return;
    }

    if (t->looping && ev == t->loop_end && t->loop_remaining != 0) {
        // Jump back to the start of the loop. Notes still sounding when it ends keep their
        // note-offs, which are after the loop end in the event list, so move them aside.
        MIDI_EVENT *e;

        for (e = ev->next; e; e = e->next)
            if ((e->status & 0xF0) == 0x90 && e->data[1] == 0)
                add_pending_off(t, e->time + t->offset, e);
        if (t->loop_remaining > 0)
            t->loop_remaining--;
        t->offset += t->loop_length;
        t->event = t->loop_start->next;
        return;
    }

    if (t->is_master && (ev->status & ~15) == 0xB0 && ev->data[0] == 119) {
        if (ev->data[1])
            apply_requests(t->start_us + aim); // start of a bar
        else
            SDL_AtomicSet(&AiDue, 1); // time to work out what the next bar will play
    } else {
        dispatch_event(i, ev);
    }
    t->event = ev->next;
}

int MyThread(void *arg) {
    int i;

    // timing of the notes depends on this thread waking up when it should
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL);

    for (i = 0; i < NUM_THREADS; i++) {
        SlotPiece[i] = -1;
        SDL_AtomicSet(&ThreadPlaying[i], 0);
        SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
    }

    for (;;) {
        int delay = 1;

        for (i = 0; i < NUM_THREADS; i++) {
            SeqThread *t = &Seq[i];
            int command = SDL_AtomicGet(&ThreadCommand[i]);
            int n;

            if (command == THREAD_EXIT)
                return 0;

            if (command == THREAD_STOPTRACK) {
                delay = 0;
                seq_stop(i);
                SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
            } else if (command == THREAD_PLAYTRACK) {
                delay = 0;
                seq_start(i, ThreadReqTrack[i], ThreadReqLoop[i], now_us(), 1000, 0);
                SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
            }

            if (t->ramp_len_us > 0)
                update_ramp(i, now_us());

            // send every event that is due; the next event decides how long to sleep
            for (n = 0; n < 1024 && SDL_AtomicGet(&ThreadPlaying[i]); n++) {
                MIDI_EVENT *ev = t->event;
                int time, is_pending = 0;
                double aim, diff;

                if (t->npending && (!ev || t->pending[0].time <= ev->time + t->offset)) {
                    is_pending = 1;
                    ev = t->pending[0].ev;
                    time = t->pending[0].time;
                } else if (ev) {
                    time = ev->time + t->offset;
                } else {
                    seq_stop(i); // nothing left to play
                    break;
                }

                aim = t->last_time + (time - t->last_tick) * TICK_US;
                diff = aim - (now_us() - t->start_us);
                if (diff > 0) {
                    if (diff < 2500.0)
                        delay = 0; // close: spin rather than risk oversleeping
                    break;
                }
                delay = 0;

                t->last_tick = time;
                t->last_time = aim;

                if (is_pending) {
                    t->npending--;
                    memmove(&t->pending[0], &t->pending[1], t->npending * sizeof(PendingOff));
                }
                process_event(i, ev, is_pending, aim);
            }
        }

        SDL_Delay(delay);
    }

    return 0;
}

int GetTrackNumChannels(unsigned int track) {
    int num = 0, channel;

    // count channels used by track (could be zero if only percussion channel (9) is used)
    for (channel = 0; channel < 16; channel++)
        if (channel != 9 && (TrackUsedChannels[track] & (1 << channel)))
            num++;

    return num;
}

static void post_start(int thread, unsigned int track, int loop) {
    if (thread < 0 || thread >= NUM_THREADS || track >= NumTracks)
        return;

    while (SDL_AtomicGet(&ThreadCommand[thread]) != THREAD_READY)
        SDL_Delay(1);

    ThreadReqTrack[thread] = track;
    ThreadReqLoop[thread] = loop;
    SDL_AtomicSet(&ThreadCommand[thread], THREAD_PLAYTRACK);

    while (SDL_AtomicGet(&ThreadCommand[thread]) != THREAD_READY)
        SDL_Delay(1);
}

// Play a track once through.
void StartTrack(int thread, unsigned int track) { post_start(thread, track, 0); }

// Play a track, looping between its XMIDI loop controllers (if it has any) until stopped.
void StartTrackLooped(int thread, unsigned int track) { post_start(thread, track, 1); }

// Start the theme's master track (track 0). Its triggers start and stop the pieces set with
// XmiSetRequests, one bar at a time, and tell XmiConsumeAiDue when to work out the next bar.
void StartMasterClock(void) {
    if (NumTracks < 2)
        return;
    SDL_AtomicSet(&AiDue, 0);
    post_start(MASTER_THREAD, 0, 1);
}

void StopTrack(int i) {
    if (!SDL_AtomicGet(&ThreadPlaying[i]))
        return;

    while (SDL_AtomicGet(&ThreadCommand[i]) != THREAD_READY)
        SDL_Delay(1);

    SDL_AtomicSet(&ThreadCommand[i], THREAD_STOPTRACK);

    while (SDL_AtomicGet(&ThreadCommand[i]) != THREAD_READY)
        SDL_Delay(1);
}

void StopTheMusic(void) {
    int i;

    // the master first, so that it can't start anything else
    for (i = NUM_THREADS - 1; i >= 0; i--)
        StopTrack(i);

    SDL_AtomicSet(&AiDue, 0);

    SDL_LockMutex(MyMutex);
    if (MusicDev) {
        MusicDev->reset(MusicDev);
    }
    SDL_UnlockMutex(MyMutex);
}

int IsPlaying(int i) { return SDL_AtomicGet(&ThreadPlaying[i]); }

// The pieces the music AI wants playing from the next bar on (piece -1 = unused entry).
void XmiSetRequests(const XmiRequest *requests, int count) {
    int i;

    SDL_LockMutex(RequestMutex);
    for (i = 0; i < MAX_PIECES; i++) {
        if (i < count) {
            Requests[i] = requests[i];
        } else {
            memset(&Requests[i], 0, sizeof(Requests[i]));
            Requests[i].piece = -1;
        }
    }
    SDL_UnlockMutex(RequestMutex);
}

// True when the pieces of the theme have all been faded out (or there are none playing).
int XmiThemeFaded(void) {
    int s;

    for (s = 0; s < MAX_PIECES; s++)
        if (SDL_AtomicGet(&ThreadPlaying[s]) && Seq[s].vol1000 > 0)
            return 0;
    return 1;
}

// True once per bar, when the master says it's time to work out the next bar.
int XmiConsumeAiDue(void) { return SDL_AtomicSet(&AiDue, 0); }

// Bars per piece and per master loop; read from the theme's .dat file.
void XmiSetPieceMeasures(const unsigned char *measures, int count, int master_measures) {
    memset(PieceMeasures, 0, sizeof(PieceMeasures));
    if (measures)
        memcpy(PieceMeasures, measures, count > 256 ? 256 : count);
    MasterMeasures = master_measures > 0 ? master_measures : 1;
}

void InitReadXMI(void) {
    int channel, i;
    SDL_Thread *thread;

    InitDecXMI();

    MyMutex = SDL_CreateMutex();
    RequestMutex = SDL_CreateMutex();
    for (i = 0; i < NUM_THREADS; i++)
        Requests[i].piece = -1;

    for (channel = 0; channel < 16; channel++) {
        ChannelThread[channel] = -1;
        SDL_AtomicSet(&DeviceChannelVolume[channel], 0);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        SDL_AtomicSet(&ThreadPlaying[i], 0);
        SDL_AtomicSet(&ThreadCommand[i], THREAD_INIT);
    }

    thread = SDL_CreateThread(MyThread, "MyThread", NULL);
    SDL_DetachThread(thread); // thread will go away on its own upon completion

    i = 0;
    while (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_INIT)
        SDL_Delay(1);

    atexit(ShutdownReadXMI);
}

void InitDecXMI(void) {
    SDL_LockMutex(MyMutex);
    if (MusicDev) {
        WARN("InitDecXMI(): *****WARNING***** Creating new music device, but one already exists!");
    }

    // Start the Midi device
    MusicDevice *musicdev = NULL;
    int musicrate = 48000;

    switch (gShockPrefs.soMidiBackend) {
    case OPT_SEQ_ADLMIDI: // adlmidi
    {
        INFO("Creating ADLMIDI device");
        musicdev = CreateMusicDevice(Music_AdlMidi);
    } break;
    case OPT_SEQ_NativeMI: // native midi
    {
        INFO("Creating native MIDI device");
        musicdev = CreateMusicDevice(Music_Native);
    } break;
#ifdef USE_FLUIDSYNTH
    case OPT_SEQ_FluidSyn: // fluidsynth
    {
        INFO("Creating FluidSynth MIDI device");
        musicdev = CreateMusicDevice(Music_FluidSynth);
    } break;
#endif
    }

    // init chosen music device
    INFO("Opening MIDI device using output %d", gShockPrefs.soMidiOutput);
    if (musicdev && musicdev->init(musicdev, gShockPrefs.soMidiOutput, musicrate) != 0) {
        musicdev->destroy(musicdev);
        musicdev = NULL;
    }

    // fallback to dummy
    if (!musicdev) {
        WARN("Using dummy MIDI driver");
        musicdev = CreateMusicDevice(Music_None);
        if (musicdev) {
            musicdev->init(musicdev, gShockPrefs.soMidiOutput, musicrate);
        }
    }

    // force prefs to align with music device output
    if (musicdev) {
        gShockPrefs.soMidiOutput = musicdev->outputIndex;
    }

    MusicDev = musicdev;
    SDL_UnlockMutex(MyMutex);
}

void ReloadDecXMI(void) {
    int i;

    // determine whether a device type change is being requested, by comparing the
    //  current device type (if any) with current preferences setting
    short deviceTypeMatch = 0;
    SDL_LockMutex(MyMutex);
    if (MusicDev) {
        switch (MusicDev->deviceType) {
        case Music_None:
            deviceTypeMatch = 0;
            break;
        case Music_AdlMidi:
            deviceTypeMatch = (gShockPrefs.soMidiBackend == 0);
            break;
        case Music_Native:
            deviceTypeMatch = (gShockPrefs.soMidiBackend == 1);
            break;
#ifdef USE_FLUIDSYNTH
        case Music_FluidSynth:
            deviceTypeMatch = (gShockPrefs.soMidiBackend == 2);
            break;
#endif
        }
    }

    // only destroy the device if it exists and any of the following apply:
    // - it hasn't been opened yet
    // - device type change requested
    // - device output change requested
    // this is needed to protect against reload spam generated by the UI slider
    if (MusicDev && (!MusicDev->isOpen || !deviceTypeMatch || MusicDev->outputIndex != gShockPrefs.soMidiOutput)) {
        SDL_UnlockMutex(MyMutex);
        INFO("Closing MIDI driver due to reload");

        for (i = 0; i < NUM_THREADS; i++) {
            StopTrack(i);
        }

        SDL_LockMutex(MyMutex);
        MusicDev->destroy(MusicDev);
        MusicDev = NULL;
    }

    // only init music device if it doesn't still exist
    if (!MusicDev) {
        SDL_UnlockMutex(MyMutex);
        InitDecXMI();
    } else {
        SDL_UnlockMutex(MyMutex);
    }
}

void ShutdownReadXMI(void) {
    int i;

    for (i = 0; i < NUM_THREADS; i++) {
        StopTrack(i);
        // don't set THREAD_EXIT yet, as this seems to cause deadlocks
    }
    SDL_Delay(50);
    for (i = 0; i < NUM_THREADS; i++) {
        SDL_AtomicSet(&ThreadCommand[i], THREAD_EXIT);
    }
    SDL_Delay(50); // wait a bit for thread to hopefully exit

    INFO("Closing MIDI driver due to shutdown");
    SDL_LockMutex(MyMutex);
    if (MusicDev) {
        MusicDev->destroy(MusicDev);
        MusicDev = NULL;
    } else {
        WARN("ShutdownReadXMI(): Shutdown request received, but no music device exists!");
    }
    SDL_UnlockMutex(MyMutex);

    FreeXMI();

    SDL_DestroyMutex(MyMutex);
    SDL_DestroyMutex(RequestMutex);
}

unsigned int GetOutputCountXMI(void) {
    unsigned int outputCount = 0;

    SDL_LockMutex(MyMutex);
    if (MusicDev) {
        outputCount = MusicDev->getOutputCount(MusicDev);
    }
    SDL_UnlockMutex(MyMutex);

    return outputCount;
}

void GetOutputNameXMI(const unsigned int outputIndex, char *buffer, const unsigned int bufferSize) {
    SDL_LockMutex(MyMutex);
    if (MusicDev && buffer && bufferSize >= 1) {
        MusicDev->getOutputName(MusicDev, outputIndex, buffer, bufferSize);
    }
    SDL_UnlockMutex(MyMutex);
}

void UpdateVolumeXMI(void) {
    // global volume has been changed
    extern uchar curr_vol_lev; // 0-100
    INFO("UpdateVolumeXMI(): Global music volume change to %d percent", curr_vol_lev);

    // tell the music driver
    SDL_LockMutex(MyMutex);
    if (MusicDev && MusicDev->isOpen) {
        // send volume change controller (#7) for all channels
        for (int i = 0; i <= 15; ++i) {
            // skip unused device channels
            if (i != 9 && ChannelThread[i] == -1)
                continue;
            // scale new volume according to global music volume
            int scaledVolume = ((int)SDL_AtomicGet(&DeviceChannelVolume[i]) * (int)curr_vol_lev) / 100;
            if (i != 9 && ChannelThread[i] >= 0)
                scaledVolume = (scaledVolume * Seq[(int)ChannelThread[i]].vol1000) / 1000;
            MusicDev->sendControllerChange(MusicDev, i, 7, scaledVolume);
        }
    }
    SDL_UnlockMutex(MyMutex);
}
