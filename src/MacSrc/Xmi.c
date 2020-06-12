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

MIDI_EVENT *ThreadEventList[NUM_THREADS];
int ThreadTiming[NUM_THREADS];
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

int MyThread(void *arg) {
    int i;

    MIDI_EVENT *event[NUM_THREADS];
    int ppqn[NUM_THREADS];
    double Ippqn[NUM_THREADS];
    int tempo[NUM_THREADS];
    double tick[NUM_THREADS];
    double last_tick[NUM_THREADS];
    double last_time[NUM_THREADS];
    unsigned int start[NUM_THREADS];

    for (i = 0; i < NUM_THREADS; i++) {
        event[i] = 0;
        ppqn[i] = 1;
        Ippqn[i] = 1;
        tempo[i] = 0x07A120;
        tick[i] = 1;
        last_tick[i] = 0;
        last_time[i] = 0;
        start[i] = 0;

        SDL_AtomicSet(&ThreadPlaying[i], 0);
        SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
    }

    for (;;) {
        int delay = 1;

        for (i = 0; i < NUM_THREADS; i++) {
            if (event[i] && SDL_AtomicGet(&ThreadCommand[i]) != THREAD_STOPTRACK) {
                double aim = last_time[i] + (event[i]->time - last_tick[i]) * tick[i];
                double diff = aim - ((SDL_GetTicks() - start[i]) * 1000.0);

                if (diff > 0) {
                    if (diff < 1200.0)
                        delay = 0;
                    continue;
                }
                delay = 0;

                last_tick[i] = event[i]->time;
                last_time[i] = aim;

                if (event[i]->status == 0xFF && event[i]->data[0] == 0x51) // tempo change
                {
                    tempo[i] = (event[i]->buffer[0] << 16) | (event[i]->buffer[1] << 8) | event[i]->buffer[2];
                    tick[i] = tempo[i] * Ippqn[i];
                } else if (event[i]->status >= 0x80 && event[i]->status < 0xF0) {
                    int channel = (event[i]->status & 15);

                    if (channel != 9)
                        channel = ThreadChannelRemap[channel + 16 * i]; // remap channel, except 9 (percussion)

                    uint8_t p1 = event[i]->data[0];
                    uint8_t p2 = event[i]->data[1];

                    if ((event[i]->status & ~15) == 0xB0 && event[i]->data[0] == 0x07) {
                        // set volume msb
                        // store in array in case global volume changes later
                        SDL_AtomicSet(&DeviceChannelVolume[channel], p2); // 0-127
                        // send volume change to device
                        SDL_LockMutex(MyMutex);
                        if (MusicDev && MusicDev->isOpen) {
                            // scale new volume according to global music volume
                            extern uchar curr_vol_lev; // 0-100
                            const int scaledVolume = ((int)p2 * (int)curr_vol_lev) / 100;
                            MusicDev->sendControllerChange(MusicDev, channel, p1, scaledVolume);
                        }
                        SDL_UnlockMutex(MyMutex);
                    } else {
                        SDL_LockMutex(MyMutex);
                        if (MusicDev && MusicDev->isOpen) {
                            switch (event[i]->status & ~15) {
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
                }

                event[i] = event[i]->next;
                if (event[i] == 0)
                    SDL_AtomicSet(&ThreadCommand[i], THREAD_STOPTRACK);
            }

            if (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_STOPTRACK) {
                int channel;

                delay = 0;

                event[i] = 0;
                last_tick[i] = 0;
                last_time[i] = 0;
                start[i] = SDL_GetTicks();

                // here we should turn off all notes for these channels plus 9 (percussion)
                for (channel = 0; channel < 16; channel++)
                    if (ChannelThread[channel] == i) {
                        ChannelThread[channel] = -1;
                        NumUsedChannels--;
                        SDL_AtomicSet(&DeviceChannelVolume[channel], 0);
                    }

                SDL_AtomicSet(&ThreadPlaying[i], 0);
                SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
            }

            if (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_PLAYTRACK) {
                delay = 0;

                event[i] = ThreadEventList[i];
                ppqn[i] = ThreadTiming[i];
                Ippqn[i] = 1.0 / ppqn[i];
                tempo[i] = 0x07A120;
                tick[i] = tempo[i] * Ippqn[i];
                last_tick[i] = 0;
                last_time[i] = 0;
                start[i] = SDL_GetTicks();

                SDL_AtomicSet(&ThreadPlaying[i], 1);
                SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
            }

            if (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_EXIT)
                return 0;
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

void StartTrack(int thread, unsigned int track) {
    int num, trackChannel, deviceChannel;
    char channel_remap[16];

    if (track >= NumTracks)
        return;

    num = GetTrackNumChannels(track);

    while (SDL_AtomicGet(&ThreadCommand[thread]) != THREAD_READY)
        SDL_Delay(1);

    // check if enough device channels free; 16 channels available except one (percussion)
    if (NumUsedChannels + num <= 16 - 1) {
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

        ThreadEventList[thread] = TrackEvents[track];
        ThreadTiming[thread] = TrackTiming[track];
        memcpy(ThreadChannelRemap + 16 * thread, channel_remap, 16);

        SDL_AtomicSet(&ThreadCommand[thread], THREAD_PLAYTRACK);

        while (SDL_AtomicGet(&ThreadCommand[thread]) != THREAD_READY)
            SDL_Delay(1);
    }
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

    for (i = 0; i < NUM_THREADS; i++)
        StopTrack(i);

    SDL_LockMutex(MyMutex);
    if (MusicDev) {
        MusicDev->reset(MusicDev);
    }
    SDL_UnlockMutex(MyMutex);
}

int IsPlaying(int i) { return SDL_AtomicGet(&ThreadPlaying[i]); }

void InitReadXMI(void) {
    int channel, i;
    SDL_Thread *thread;

    InitDecXMI();

    MyMutex = SDL_CreateMutex();

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
            const int scaledVolume = ((int)SDL_AtomicGet(&DeviceChannelVolume[i]) * (int)curr_vol_lev) / 100;
            MusicDev->sendControllerChange(MusicDev, i, 7, scaledVolume);
        }
    }
    SDL_UnlockMutex(MyMutex);
}
