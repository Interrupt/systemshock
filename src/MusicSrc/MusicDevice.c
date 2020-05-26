#include "MusicDevice.h"
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
// General Windows API support
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
// Windows NativeMidi backend support
# include <mmsystem.h>
#else
// Linux NativeMidi backend support
# if defined(USE_ALSA)
#  include <alsa/asoundlib.h>
# endif
// Linux/Mac FluidMidi SF2 search support
# include <sys/types.h>
# include <dirent.h>
#endif

//------------------------------------------------------------------------------
// Dummy MIDI player

static int NullMidiInit(MusicDevice *dev, const unsigned int outputIndex, unsigned samplerate)
{
    if (!dev || dev->isOpen) return 0;

    dev->isOpen = 1;
    dev->outputIndex = 0;

    // suppress compiler warnings
    (void)outputIndex;
    (void)samplerate;

    return 0;
}

static void NullMidiDestroy(MusicDevice *dev)
{
    if (!dev) return;

    free(dev);
}

static void NullMidiSetupMode(MusicDevice *dev, MusicMode mode)
{
    // suppress compiler warnings
    (void)dev;
    (void)mode;
}

static void NullMidiReset(MusicDevice *dev)
{
    // suppress compiler warnings
    (void)dev;
}

static void NullMidiGenerate(MusicDevice *dev, short *samples, int numframes)
{
    memset(samples, 0, 2 * (unsigned int)numframes * sizeof(short));

    // suppress compiler warnings
    (void)dev;
}

static void NullMidiSendNoteOff(MusicDevice *dev, int channel, int note, int vel)
{
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)note;
    (void)vel;
}

static void NullMidiSendNoteOn(MusicDevice *dev, int channel, int note, int vel)
{
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)note;
    (void)vel;
}

static void NullMidiSendNoteAfterTouch(MusicDevice *dev, int channel, int note, int touch)
{
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)note;
    (void)touch;
}

static void NullMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val)
{
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)ctl;
    (void)val;
}

static void NullMidiSendProgramChange(MusicDevice *dev, int channel, int pgm)
{
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)pgm;
}

static void NullMidiSendChannelAfterTouch(MusicDevice *dev, int channel, int touch)
{
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)touch;
}

static void NullMidiSendPitchBendML(MusicDevice *dev, int channel, int msb, int lsb)
{
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)msb;
    (void)lsb;
}

static unsigned int NullMidiGetOutputCount(MusicDevice *dev)
{
    // suppress compiler warnings
    (void)dev;

    return 1;
}

static void NullMidiGetOutputName(MusicDevice *dev, const unsigned int outputIndex, char *buffer, const unsigned int bufferSize)
{
    if (!buffer || bufferSize < 1) return;
    // save last position for NULL character
    strncpy(buffer, "NullMidi", bufferSize - 1);
    // put NULL in last position in case we filled up everything else
    *(buffer + bufferSize - 1) = '\0';

    // suppress compiler warnings
    (void)dev;
    (void)outputIndex;
}

static MusicDevice *createNullMidiDevice()
{
    MusicDevice *dev = malloc(sizeof(MusicDevice));
    dev->init = &NullMidiInit;
    dev->destroy = &NullMidiDestroy;
    dev->setupMode = &NullMidiSetupMode;
    dev->reset = &NullMidiReset;
    dev->generate = &NullMidiGenerate;
    dev->sendNoteOff = &NullMidiSendNoteOff;
    dev->sendNoteOn = &NullMidiSendNoteOn;
    dev->sendNoteAfterTouch = &NullMidiSendNoteAfterTouch;
    dev->sendControllerChange = &NullMidiSendControllerChange;
    dev->sendProgramChange = &NullMidiSendProgramChange;
    dev->sendChannelAfterTouch = &NullMidiSendChannelAfterTouch;
    dev->sendPitchBendML = &NullMidiSendPitchBendML;
    dev->getOutputCount = &NullMidiGetOutputCount;
    dev->getOutputName = &NullMidiGetOutputName;
    dev->isOpen = 0;
    dev->outputIndex = 0;
    dev->deviceType = Music_None;
    dev->musicType = MUSICTYPE_SBLASTER;
    return dev;
}

//------------------------------------------------------------------------------
// ADLMIDI player for OPL3

#include "adlmidi.h"

typedef struct AdlMidiDevice
{
    MusicDevice dev;
    struct ADL_MIDIPlayer *adl;
} AdlMidiDevice;

static int AdlMidiInit(MusicDevice *dev, const unsigned int outputIndex, unsigned samplerate)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || adev->dev.isOpen) return 0;
    struct ADL_MIDIPlayer *adl = adl_init(samplerate);

    adl_switchEmulator(adl, ADLMIDI_EMU_NUKED_174);
    adl_setNumChips(adl, 1);
    adl_setVolumeRangeModel(adl, ADLMIDI_VolumeModel_AUTO);
    adl_setRunAtPcmRate(adl, 1);

    adev->adl = adl;

    adev->dev.isOpen = 1;
    adev->dev.outputIndex = outputIndex;

    return 0;
}

static void AdlMidiDestroy(MusicDevice *dev)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev) return;
    if (adev->dev.isOpen)
    {
        adl_close(adev->adl);
    }
    free(adev);
}

static void AdlMidiSetupMode(MusicDevice *dev, MusicMode mode)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    //Use sound bank 45 for res/sound/sblaster, 0 for res/sound/genmidi
    adl_setBank(adev->adl, (mode == Music_SoundBlaster) ? 45 : 0);
}

static void AdlMidiReset(MusicDevice *dev)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_reset(adev->adl);
}

static void AdlMidiGenerate(MusicDevice *dev, short *samples, int numframes)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    const int numSamples = numframes * 2;
    adl_generate(adev->adl, numSamples, samples);
    // ugly hack: libadlmidi has quiet output, so double all values
    short *sample = samples;
    for (int i = 0; i < numSamples; ++i, ++sample)
    {
        *sample *= 2;
    }
}

static void AdlMidiSendNoteOff(MusicDevice *dev, int channel, int note, int vel)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_rt_noteOff(adev->adl, channel, note);
    (void)vel;
}

static void AdlMidiSendNoteOn(MusicDevice *dev, int channel, int note, int vel)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_rt_noteOn(adev->adl, channel, note, vel);
}

static void AdlMidiSendNoteAfterTouch(MusicDevice *dev, int channel, int note, int touch)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_rt_noteAfterTouch(adev->adl, channel, note, touch);
}

static void AdlMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_rt_controllerChange(adev->adl, channel, ctl, val);
}

static void AdlMidiSendProgramChange(MusicDevice *dev, int channel, int pgm)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_rt_patchChange(adev->adl, channel, pgm);
}

static void AdlMidiSendChannelAfterTouch(MusicDevice *dev, int channel, int touch)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_rt_channelAfterTouch(adev->adl, channel, touch);
}

static void AdlMidiSendPitchBendML(MusicDevice *dev, int channel, int msb, int lsb)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    if (!adev || !adev->dev.isOpen) return;

    adl_rt_pitchBendML(adev->adl, channel, msb, lsb);
}

static unsigned int AdlMidiGetOutputCount(MusicDevice *dev)
{
    // suppress compiler warnings
    (void)dev;

    // TODO: add support for SB and GM modes?
    return 1;
}

static void AdlMidiGetOutputName(MusicDevice *dev, const unsigned int outputIndex, char *buffer, const unsigned int bufferSize)
{
    if (!buffer || bufferSize < 1) return;
    // save last position for NULL character
    strncpy(buffer, "AdlMidi", bufferSize - 1);
    // put NULL in last position in case we filled up everything else
    *(buffer + bufferSize - 1) = '\0';

    // suppress compiler warnings
    (void)dev;
    (void)outputIndex;
}

static MusicDevice *createAdlMidiDevice()
{
    AdlMidiDevice *adev = malloc(sizeof(AdlMidiDevice));
    adev->dev.init = &AdlMidiInit;
    adev->dev.destroy = &AdlMidiDestroy;
    adev->dev.setupMode = &AdlMidiSetupMode;
    adev->dev.reset = &AdlMidiReset;
    adev->dev.generate = &AdlMidiGenerate;
    adev->dev.sendNoteOff = &AdlMidiSendNoteOff;
    adev->dev.sendNoteOn = &AdlMidiSendNoteOn;
    adev->dev.sendNoteAfterTouch = &AdlMidiSendNoteAfterTouch;
    adev->dev.sendControllerChange = &AdlMidiSendControllerChange;
    adev->dev.sendProgramChange = &AdlMidiSendProgramChange;
    adev->dev.sendChannelAfterTouch = &AdlMidiSendChannelAfterTouch;
    adev->dev.sendPitchBendML = &AdlMidiSendPitchBendML;
    adev->dev.getOutputCount = &AdlMidiGetOutputCount;
    adev->dev.getOutputName = &AdlMidiGetOutputName;
    adev->dev.isOpen = 0;
    adev->dev.outputIndex = 0;
    adev->dev.deviceType = Music_AdlMidi;
    adev->dev.musicType = MUSICTYPE_SBLASTER;
    return &adev->dev;
}

//------------------------------------------------------------------------------
// Native OS MIDI
//
// Currently only supports Windows MCI MIDI
// could support coremidi on OSX in the future?
// this devolves into another null driver on unsupported configurations

typedef struct
{
    MusicDevice dev;
#ifdef WIN32
    HMIDIOUT outHandle;
#elif defined(USE_ALSA)
    snd_seq_t *outHandle;
    int alsaMyId;
    int alsaMyPort;
    int alsaOutputId;
    int alsaOutputPort;
#endif
} NativeMidiDevice;

// all standard MIDI message types
// these go in the high nibble of status byte
// low nibble is used for channel #
// source: http://midi.teragonaudio.com/tech/midispec.htm
typedef enum
{
    MME_NOTE_OFF         = 0x8,
    MME_NOTE_ON          = 0x9,
    MME_AFTERTOUCH       = 0xA,
    MME_CONTROL_CHANGE   = 0xB,
    MME_PROGRAM_CHANGE   = 0xC,
    MME_CHANNEL_PRESSURE = 0xD,
    MME_PITCH_WHEEL      = 0xE
} MidiMessageEnum;

// data1 byte for MSE_CONTROL_CHANGE message
// this is a relevant subset, because there are dozens
// source: http://midi.teragonaudio.com/tech/midispec.htm
typedef enum
{
    MCE_ALL_SOUND_OFF       = 120,
    MCE_ALL_CONTROLLERS_OFF = 121
} MidiControllerEnum;

// define backend-API-specific helper macros here
#define NM_CLAMP15(x)  ((unsigned char)((unsigned char)(x) & 0x0F))
#define NM_CLAMP127(x) ((unsigned char)((unsigned char)(x) & 0x7F))
#define NM_CLAMP255(x) ((unsigned char)((unsigned char)(x) & 0xFF))

// define backend-API-specific helper functions here
#ifdef WIN32
inline static void NativeMidiSendMessage(
    HMIDIOUT outHandle,
    const MidiMessageEnum message,
    const UCHAR channel,
    const UCHAR data1,
    const UCHAR data2)
{
    union {
        DWORD dwData;
        UCHAR bData[4];
    } u;
    u.bData[0] = (UCHAR)(NM_CLAMP15(message) << 4 | NM_CLAMP15(channel));
    u.bData[1] = NM_CLAMP127(data1);
    u.bData[2] = NM_CLAMP127(data2);
    u.bData[3] = 0;

//    INFO("NativeMidiSendMessage(): Sending MIDI data: 0x%08X", u.dwData);
    const unsigned long err = midiOutShortMsg(outHandle, u.dwData);
    if (err)
    {
        static char buffer[1024];
        midiOutGetErrorText(err, &buffer[0], 1024);
        WARN("NativeMidiSendMessage(): midiOutShortMsg() error: %s", &buffer[0]);
    }
}
#elif defined(USE_ALSA)
inline static void NativeMidiAlsaInitEvent(NativeMidiDevice *ndev, snd_seq_event_t* ev)
{
    if (!ndev || !ndev->dev.isOpen || !ev) return;
    snd_seq_ev_clear(ev);
    snd_seq_ev_set_direct(ev); // do it now
    snd_seq_ev_set_source(ev, ndev->alsaMyPort);
    snd_seq_ev_set_dest(ev, ndev->alsaOutputId, ndev->alsaOutputPort);
}

inline static void NativeMidiAlsaSendEvent(NativeMidiDevice *ndev, snd_seq_event_t* ev)
{
    if (!ndev || !ndev->dev.isOpen || !ev) return;
    snd_seq_event_output(ndev->outHandle, ev); // send to queue
    snd_seq_drain_output(ndev->outHandle); // process queue
}
#endif

// forward declares
static void NativeMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val);
static void NativeMidiReset(MusicDevice *dev);

static int NativeMidiInit(MusicDevice *dev, const unsigned int outputIndex, unsigned samplerate)
{
//    INFO("Native MIDI device open request for outputIndex=%d", outputIndex);
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || ndev->dev.isOpen) return 0;
#ifdef WIN32
    // if outputIndex is 0, use MIDI_MAPPER
    // else subract 1 to get the real output number
    const UINT realOutput = (
        outputIndex == 0 ? MIDI_MAPPER : outputIndex - 1);
    MMRESULT res = midiOutOpen(&(ndev->outHandle), realOutput, 0, 0, CALLBACK_NULL);
    if (res != MMSYSERR_NOERROR)
    {
        static char buffer[1024];
        midiOutGetErrorText(res, &buffer[0], 1024);
        WARN("NativeMidiInit(): native midi open failed with error: %s", &buffer[0]);
        return -1;
    }
//    INFO("NativeMidiInit(): native midi open succeeded");
    ndev->dev.isOpen = 1;
    ndev->dev.outputIndex = outputIndex;
    // send MIDI reset in case it was in a dirty state when we opened it
    NativeMidiReset(dev);
#elif defined(USE_ALSA)
    unsigned short foundOutput = 0;
    unsigned int outputCount = 0; // subtract 1 to get index
    int alsaError = 0;
    snd_seq_client_info_t *cinfo = 0;
    snd_seq_port_info_t *pinfo = 0;
    int client = 0;

    // open the sequencer interface
    if ((alsaError = snd_seq_open(&(ndev->outHandle), "default", SND_SEQ_OPEN_OUTPUT, 0)) < 0)
    {
        WARN("Error opening ALSA sequencer: %s", snd_strerror(alsaError));
        if (ndev->outHandle)
        {
            snd_seq_close(ndev->outHandle);
            ndev->outHandle = 0;
        }
        return -1;
    }

    // count ports that support MIDI write until we reach the requested index,
    //  which is probably the one we want
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (outputCount <= outputIndex &&
           snd_seq_query_next_client(ndev->outHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (outputCount <= outputIndex &&
               snd_seq_query_next_port(ndev->outHandle, pinfo) >= 0)
        {
            /* port must understand MIDI messages */
            if (!(snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
                continue;
            /* we need both WRITE and SUBS_WRITE */
            if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE)) !=
                (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
                continue;

            ++outputCount;
            if (outputCount - 1 == outputIndex)
            {
                // found it
                foundOutput = 1;
            }
        }
    }

    if (!foundOutput)
    {
        WARN("Failed to locate ALSA MIDI output at outputIndex=%d", outputIndex);
        // close the sequencer interface
        snd_seq_close(ndev->outHandle);
        ndev->outHandle = 0;
        return -1;
    }

    // get client ID
    ndev->alsaMyId = snd_seq_client_id(ndev->outHandle);
    if ((alsaError = snd_seq_set_client_name(ndev->outHandle, "Shockolate")) < 0)
    {
        WARN("Error setting ALSA sequencer client name: %s", snd_strerror(alsaError));
    }
    // create client port
    ndev->alsaMyPort = snd_seq_create_simple_port(
        ndev->outHandle,
        "Shockolate",
        0,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );
    if (ndev->alsaMyPort < 0)
    {
        WARN("Error creating ALSA sequencer client port: %s", snd_strerror(ndev->alsaMyPort));
        snd_seq_close(ndev->outHandle);
        ndev->outHandle = 0;
        return -1;
    }

    // connect our client to the output
    ndev->alsaOutputId = snd_seq_port_info_get_client(pinfo);
    ndev->alsaOutputPort = snd_seq_port_info_get_port(pinfo);
    if ((alsaError = snd_seq_connect_to(
        ndev->outHandle,    ndev->alsaMyPort,
        ndev->alsaOutputId, ndev->alsaOutputPort
    )) < 0)
    {
        WARN("Failed to connect ALSA MIDI device: %s", snd_strerror(alsaError));
        snd_seq_close(ndev->outHandle);
        ndev->outHandle = 0;
        return -1;
    }

    // connected
    ndev->dev.isOpen = 1;
    ndev->dev.outputIndex = outputIndex;
#endif
    // suppress compiler warnings
    (void)outputIndex;
    (void)samplerate;

    return 0;
}

static void NativeMidiDestroy(MusicDevice *dev)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev) return;
#ifdef WIN32
    if (ndev->dev.isOpen)
    {
//        INFO("NativeMidiDestroy(): closing native midi");
        // reset before close, so that notes aren't left hanging
        NativeMidiReset(dev);
        midiOutClose(ndev->outHandle);
        ndev->outHandle = 0;
        ndev->dev.isOpen = 0;
    }
#elif defined(USE_ALSA)
    if (ndev->dev.isOpen)
    {
        if (ndev->outHandle)
        {
            NativeMidiReset(dev);
            snd_seq_close(ndev->outHandle);
            ndev->outHandle = 0;
            ndev->alsaMyId = 0;
            ndev->alsaMyPort = 0;
            ndev->alsaOutputId = 0;
            ndev->alsaOutputPort = 0;
        }
        ndev->dev.isOpen = 0;
    }
#endif
    free(ndev);
}

static void NativeMidiSetupMode(MusicDevice *dev, MusicMode mode)
{
    // nothing to do

    // suppress compiler warnings
    (void)dev;
    (void)mode;
}

static void NativeMidiReset(MusicDevice *dev)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#if defined(WIN32) || defined(USE_ALSA)
    // send All Sound Off for all channels
    for (unsigned char chan = 0; chan <= 15; ++chan)
    {
        NativeMidiSendControllerChange(dev, chan, MCE_ALL_SOUND_OFF, 0);
    }
    // send All Controllers Off for all channels
    // this is done in a separate loop to give the previous one a chance to
    //  settle out
    for (unsigned char chan = 0; chan <= 15; ++chan)
    {
        NativeMidiSendControllerChange(dev, chan, MCE_ALL_CONTROLLERS_OFF, 0);
    }
#endif
}

static void NativeMidiGenerate(MusicDevice *dev, short *samples, int numframes)
{
    // native MIDI outputs at the OS level, to an external driver or real synth
    // generate an empty sample since we have nothing to mix at the game level
    memset(samples, 0, 2 * (unsigned int)numframes * sizeof(short));

    // suppress compiler warnings
    (void)dev;
}

static void NativeMidiSendNoteOff(MusicDevice *dev, int channel, int note, int vel)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#ifdef WIN32
    // send note off
    // yes, velocity is potentially relevant
    NativeMidiSendMessage(ndev->outHandle,
                          MME_NOTE_OFF,
                          NM_CLAMP15(channel),
                          NM_CLAMP127(note),
                          NM_CLAMP127(vel));
#elif defined(USE_ALSA)
    // send note off
    snd_seq_event_t ev;
    NativeMidiAlsaInitEvent(ndev, &ev);
    snd_seq_ev_set_noteoff(&ev, channel, note, vel);
    NativeMidiAlsaSendEvent(ndev, &ev);
#else
    // suppress compiler warnings
    (void)channel;
    (void)note;
    (void)vel;
#endif
}

static void NativeMidiSendNoteOn(MusicDevice *dev, int channel, int note, int vel)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#ifdef WIN32
    // send note on
    NativeMidiSendMessage(ndev->outHandle,
                          MME_NOTE_ON,
                          NM_CLAMP15(channel),
                          NM_CLAMP127(note),
                          NM_CLAMP127(vel));
#elif defined(USE_ALSA)
    // send note on
    snd_seq_event_t ev;
    NativeMidiAlsaInitEvent(ndev, &ev);
    snd_seq_ev_set_noteon(&ev, channel, note, vel);
    NativeMidiAlsaSendEvent(ndev, &ev);
#else
    // suppress compiler warnings
    (void)channel;
    (void)note;
    (void)vel;
#endif
}

static void NativeMidiSendNoteAfterTouch(MusicDevice *dev, int channel, int note, int touch)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#ifdef WIN32
    // send note aftertouch (pressure)
    NativeMidiSendMessage(ndev->outHandle,
                          MME_AFTERTOUCH,
                          NM_CLAMP15(channel),
                          NM_CLAMP127(note),
                          NM_CLAMP127(touch));
#elif defined(USE_ALSA)
    // send note aftertouch (pressure)
    snd_seq_event_t ev;
    NativeMidiAlsaInitEvent(ndev, &ev);
    snd_seq_ev_set_keypress(&ev, channel, note, touch);
    NativeMidiAlsaSendEvent(ndev, &ev);
#else
    // suppress compiler warnings
    (void)channel;
    (void)note;
    (void)touch;
#endif
}

static void NativeMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#ifdef WIN32
    // send controller change
    NativeMidiSendMessage(ndev->outHandle,
                          MME_CONTROL_CHANGE,
                          NM_CLAMP15(channel),
                          NM_CLAMP127(ctl),
                          NM_CLAMP127(val));
#elif defined(USE_ALSA)
    // send controller change
    snd_seq_event_t ev;
    NativeMidiAlsaInitEvent(ndev, &ev);
    snd_seq_ev_set_controller(&ev, channel, ctl, val);
    NativeMidiAlsaSendEvent(ndev, &ev);
#else
    // suppress compiler warnings
    (void)channel;
    (void)ctl;
    (void)val;
#endif
}

static void NativeMidiSendProgramChange(MusicDevice *dev, int channel, int pgm)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#ifdef WIN32
    // send program change
    // only one data byte is used
    NativeMidiSendMessage(ndev->outHandle,
                          MME_PROGRAM_CHANGE,
                          NM_CLAMP15(channel),
                          NM_CLAMP127(pgm),
                          0);
#elif defined(USE_ALSA)
    // send program change
    snd_seq_event_t ev;
    NativeMidiAlsaInitEvent(ndev, &ev);
    snd_seq_ev_set_pgmchange(&ev, channel, pgm);
    NativeMidiAlsaSendEvent(ndev, &ev);
#else
    // suppress compiler warnings
    (void)channel;
    (void)pgm;
#endif
}

static void NativeMidiSendChannelAfterTouch(MusicDevice *dev, int channel, int touch)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#ifdef WIN32
    // send channel aftertouch (pressure)
    // only one data byte is used
    NativeMidiSendMessage(ndev->outHandle,
                          MME_CHANNEL_PRESSURE,
                          NM_CLAMP15(channel),
                          NM_CLAMP127(touch),
                          0);
#elif defined(USE_ALSA)
    // send channel aftertouch (pressure)
    snd_seq_event_t ev;
    NativeMidiAlsaInitEvent(ndev, &ev);
    snd_seq_ev_set_chanpress(&ev, channel, touch);
    NativeMidiAlsaSendEvent(ndev, &ev);
#else
    // suppress compiler warnings
    (void)channel;
    (void)touch;
#endif
}

static void NativeMidiSendPitchBendML(MusicDevice *dev, int channel, int msb, int lsb)
{
    NativeMidiDevice *ndev = (NativeMidiDevice *)dev;
    if (!ndev || !ndev->dev.isOpen) return;
#ifdef WIN32
    // send pitch bend
    NativeMidiSendMessage(ndev->outHandle,
                          MME_PITCH_WHEEL,
                          NM_CLAMP15(channel),
                          NM_CLAMP127(lsb),
                          NM_CLAMP127(msb));
#elif defined(USE_ALSA)
    // send pitch bend
    snd_seq_event_t ev;
    NativeMidiAlsaInitEvent(ndev, &ev);
    // from ScummVM - seems to sound correct
    const long theBend = ((long)lsb + (long)(msb << 7)) - 0x2000;
    snd_seq_ev_set_pitchbend(&ev, channel, theBend);
    NativeMidiAlsaSendEvent(ndev, &ev);
#else
    // suppress compiler warnings
    (void)channel;
    (void)msb;
    (void)lsb;
#endif
}

static unsigned int NativeMidiGetOutputCount(MusicDevice *dev)
{
//    INFO("Native MIDI output count request");
    // suppress compiler warnings
    (void)dev;
#ifdef WIN32
    // add one for MIDI_MAPPER
    return midiOutGetNumDevs() + 1;
#elif defined(USE_ALSA)
    unsigned int outputCount = 0;
    int alsaError = 0;
    snd_seq_t *seqHandle = 0;
    snd_seq_client_info_t *cinfo = 0;
    snd_seq_port_info_t *pinfo = 0;
    int client = 0;

    // open the sequencer interface
    if ((alsaError = snd_seq_open(&seqHandle, "default", SND_SEQ_OPEN_OUTPUT, 0)) < 0)
    {
        WARN("Error opening ALSA sequencer: %s", snd_strerror(alsaError));
        return 0;
    }

    // count all ports that support MIDI write
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seqHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seqHandle, pinfo) >= 0)
        {
            /* port must understand MIDI messages */
            if (!(snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
                continue;
            /* we need both WRITE and SUBS_WRITE */
            if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE)) !=
                (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
                continue;

            ++outputCount;
        }
    }

    // close the sequencer interface
    snd_seq_close(seqHandle);

    return outputCount;
#else
    // "NULL "Unsupported" output
    return 1;
#endif
}

static void NativeMidiGetOutputName(MusicDevice *dev, const unsigned int outputIndex, char *buffer, const unsigned int bufferSize)
{
    if (!buffer || bufferSize < 1) return;
//    INFO("Native MIDI output name request for outputIndex=%d", outputIndex);
#ifdef WIN32
    if (outputIndex == 0)
    {
        // the output #0 we advertise is MIDI_MAPPER
        strncpy(buffer, "Windows MIDI mapper", bufferSize - 1);
    }
    else
    {
        // subtract one to get the real device number
        MIDIOUTCAPS moc;
        midiOutGetDevCaps(outputIndex - 1, &moc, sizeof(MIDIOUTCAPS));
        strncpy(buffer, moc.szPname, bufferSize - 1);
    }
#elif defined(USE_ALSA)
    unsigned int outputCount = 0; // subtract 1 to get index
    int alsaError = 0;
    snd_seq_t *seqHandle = 0;
    snd_seq_client_info_t *cinfo = 0;
    snd_seq_port_info_t *pinfo = 0;
    int client = 0;

    // default to nothing
    strncpy(buffer, "Device not found", bufferSize - 1);

    // open the sequencer interface
    if ((alsaError = snd_seq_open(&seqHandle, "default", SND_SEQ_OPEN_OUTPUT, 0)) < 0)
    {
        WARN("Error opening ALSA sequencer: %s", snd_strerror(alsaError));
        return;
    }

    // count ports that support MIDI write until we reach the requested index,
    //  which is probably the one we want
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (outputCount <= outputIndex &&
           snd_seq_query_next_client(seqHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        while (outputCount <= outputIndex &&
               snd_seq_query_next_port(seqHandle, pinfo) >= 0)
        {
            /* port must understand MIDI messages */
            if (!(snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC))
                continue;
            /* we need both WRITE and SUBS_WRITE */
            if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE)) !=
                (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
                continue;

            ++outputCount;
            if (outputCount - 1 == outputIndex)
            {
                // found the one we're looking for
                snprintf(buffer, bufferSize, "%d:%d %s",
                         snd_seq_port_info_get_client(pinfo),
                         snd_seq_port_info_get_port(pinfo),
                         snd_seq_client_info_get_name(cinfo));
                // don't include port name, because we need to keep it brief
                // snd_seq_port_info_get_name(pinfo));
//                INFO("MIDI outputIndex %d resolved to ALSA sequencer output %s", outputIndex, buffer);
            }
        }
    }

    // close the sequencer interface
    snd_seq_close(seqHandle);
#else
    strncpy(buffer, "Unsupported", bufferSize - 1);
    // suppress compiler warnings
    (void)outputIndex;
#endif
    // put NULL in last position in case we filled up everything else
    *(buffer + bufferSize - 1) = '\0';

    // suppress compiler warnings
    (void)dev;
}

static MusicDevice *createNativeMidiDevice()
{
    NativeMidiDevice *ndev = malloc(sizeof(NativeMidiDevice));
    ndev->dev.init = &NativeMidiInit;
    ndev->dev.destroy = &NativeMidiDestroy;
    ndev->dev.setupMode = &NativeMidiSetupMode;
    ndev->dev.reset = &NativeMidiReset;
    ndev->dev.generate = &NativeMidiGenerate;
    ndev->dev.sendNoteOff = &NativeMidiSendNoteOff;
    ndev->dev.sendNoteOn = &NativeMidiSendNoteOn;
    ndev->dev.sendNoteAfterTouch = &NativeMidiSendNoteAfterTouch;
    ndev->dev.sendControllerChange = &NativeMidiSendControllerChange;
    ndev->dev.sendProgramChange = &NativeMidiSendProgramChange;
    ndev->dev.sendChannelAfterTouch = &NativeMidiSendChannelAfterTouch;
    ndev->dev.sendPitchBendML = &NativeMidiSendPitchBendML;
    ndev->dev.getOutputCount = &NativeMidiGetOutputCount;
    ndev->dev.getOutputName = &NativeMidiGetOutputName;
    ndev->dev.isOpen = 0;
    ndev->dev.outputIndex = 0;
    ndev->dev.deviceType = Music_Native;
#ifdef WIN32
    ndev->outHandle = 0;
#elif defined(USE_ALSA)
    ndev->outHandle = 0;
    ndev->alsaMyId = 0;
    ndev->alsaMyPort = 0;
    ndev->alsaOutputId = 0;
    ndev->alsaOutputPort = 0;
#endif
    ndev->dev.musicType = MUSICTYPE_GENMIDI;
    return &(ndev->dev);
}

//------------------------------------------------------------------------------
// FluidSynth soundfont synthesizer

#ifdef USE_FLUIDSYNTH
#include <fluidsynth.h>

typedef struct FluidMidiDevice
{
    MusicDevice dev;
    fluid_synth_t *synth;
    fluid_settings_t *settings;
} FluidMidiDevice;

// forward declaration
static void FluidMidiGetOutputName(MusicDevice *dev, const unsigned int outputIndex, char *buffer, const unsigned int bufferSize);

static int FluidMidiInit(MusicDevice *dev, const unsigned int outputIndex, unsigned samplerate)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || fdev->dev.isOpen) return 0;

    fluid_settings_t *settings;
    fluid_synth_t *synth;
    int sfid;
    char fileName[1024] = "res/";

    FluidMidiGetOutputName(dev, outputIndex, &fileName[4], 1020);
    if (strlen(fileName) == 4)
    {
        WARN("Failed to locate SoundFont for outputIndex=%d", outputIndex);
        return -1;
    }

    settings = new_fluid_settings();
    fluid_settings_setnum(settings, "synth.sample-rate", samplerate);
    // default gain is 0.2, which is too conservative and ends up being quiet
    fluid_settings_setnum(settings, "synth.gain", 0.5);

    synth = new_fluid_synth(settings);
    sfid = fluid_synth_sfload(synth, fileName, 1);

    if (sfid == FLUID_FAILED)
    {
        WARN("cannot load %s for FluidSynth", fileName);
        delete_fluid_synth(synth);
        delete_fluid_settings(settings);
        fdev->synth = NULL;
        fdev->settings = NULL;
        return -1;
    }

    fluid_synth_sfont_select(synth, 0, sfid);

    fdev->synth = synth;
    fdev->settings = settings;

    fdev->dev.isOpen = 1;
    fdev->dev.outputIndex = outputIndex;

    return 0;
}

static void FluidMidiDestroy(MusicDevice *dev)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev) return;

    delete_fluid_synth(fdev->synth);
    delete_fluid_settings(fdev->settings);
    free(fdev);
}

static void FluidMidiSetupMode(MusicDevice *dev, MusicMode mode)
{
    (void)dev;
    (void)mode;
}

static void FluidMidiReset(MusicDevice *dev)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_system_reset(synth);
}

static void FluidMidiGenerate(MusicDevice *dev, short *samples, int numframes)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_write_s16(synth, numframes,
                          samples, 0, 2, /* left channel*/
                          samples, 1, 2  /* right channel*/);
}

static void FluidMidiSendNoteOff(MusicDevice *dev, int channel, int note, int vel)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_noteoff(synth, channel, note);
    (void)vel;
}

static void FluidMidiSendNoteOn(MusicDevice *dev, int channel, int note, int vel)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_noteon(synth, channel, note, vel);
}

static void FluidMidiSendNoteAfterTouch(MusicDevice *dev, int channel, int note, int touch)
{
#if FLUIDSYNTH_VERSION_MAJOR >= 2
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_key_pressure(synth, channel, note, touch);
#else
    // suppress compiler warnings
    (void)dev;
    (void)channel;
    (void)note;
    (void)touch;
#endif
}

static void FluidMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_cc(synth, channel, ctl, val);
}

static void FluidMidiSendProgramChange(MusicDevice *dev, int channel, int pgm)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_program_change(synth, channel, pgm);
}

static void FluidMidiSendChannelAfterTouch(MusicDevice *dev, int channel, int touch)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_channel_pressure(synth, channel, touch);
}

static void FluidMidiSendPitchBendML(MusicDevice *dev, int channel, int msb, int lsb)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    if (!fdev || !fdev->dev.isOpen) return;

    fluid_synth_t *synth = fdev->synth;
    fluid_synth_pitch_bend(synth, channel, msb * 128 + lsb);
}

static unsigned int FluidMidiGetOutputCount(MusicDevice *dev)
{
    unsigned int outputCount = 0;
#ifdef WIN32
    // count number of .sf2 files in res/ subdirectory
    char const * const pattern = "res\\*.sf2";
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(pattern, &data)) != INVALID_HANDLE_VALUE)
    {
        // INFO("Counting SoundFont file: %s", data.cFileName);
        do { ++outputCount; } while (FindNextFile(hFind, &data));
        FindClose(hFind);
    }
#else
    DIR *dirp = opendir("res");
    struct dirent *dp = 0;
    while ((dp = readdir(dirp)))
    {
        char *filename = dp->d_name;
        char namelen = strlen(filename);
        if (namelen < 4) continue; // ".sf2"
        if (strcasecmp(".sf2", (char*)(filename + (namelen - 4)))) continue;
        // found one
        // INFO("Counting SoundFont file: %s", filename);
        ++outputCount;
    }
    closedir(dirp);
#endif

    // suppress compiler warnings
    (void)dev;

    return outputCount;
}

static void FluidMidiGetOutputName(MusicDevice *dev, const unsigned int outputIndex, char *buffer, const unsigned int bufferSize)
{
    if (!buffer || bufferSize < 1) return;
    // default to nothing
    // save last position for NULL character
    strncpy(buffer, "No SoundFonts found", bufferSize - 1);
#if WIN32
    unsigned int outputCount = 0; // subtract 1 to get index
    // count .sf2 files in res/ subdirectory until we find the one that the user
    //  probably wants
    char const * const pattern = "res\\*.sf2";
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(pattern, &data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            ++outputCount;
            if (outputCount - 1 == outputIndex)
            {
                // found it
                strncpy(buffer, data.cFileName, bufferSize - 1);
                // INFO("Found SoundFont file for outputIndex=%d: %s", outputIndex, data.cFileName);
                break;
            }
        } while (FindNextFile(hFind, &data));
        FindClose(hFind);
    }
#else
    unsigned int outputCount = 0; // subtract 1 to get index
    // count .sf2 files in res/ subdirectory until we find the one that the user
    //  probably wants
    DIR *dirp = opendir("res");
    struct dirent *dp = 0;
    while ((outputCount <= outputIndex) &&
           (dp = readdir(dirp)))
    {
        char *filename = dp->d_name;
        char namelen = strlen(filename);
        if (namelen < 4) continue; // ".sf2"
        if (strcasecmp(".sf2", (char*)(filename + (namelen - 4)))) continue;
        // found one
        // INFO("Counting SoundFont file: %s", filename);
        ++outputCount;
        if (outputCount - 1 != outputIndex) continue;
        // found it
        strncpy(buffer, filename, bufferSize - 1);
        // INFO("Found SoundFont file for outputIndex=%d: %s", outputIndex, filename);
    }
    closedir(dirp);
#endif
    // put NULL in last position in case we filled up everything else
    *(buffer + bufferSize - 1) = '\0';

    // suppress compiler warnings
    (void)dev;
    (void)outputIndex;
}

static MusicDevice *createFluidSynthDevice()
{
    FluidMidiDevice *fdev = malloc(sizeof(FluidMidiDevice));
    fdev->dev.init = &FluidMidiInit;
    fdev->dev.destroy = &FluidMidiDestroy;
    fdev->dev.setupMode = &FluidMidiSetupMode;
    fdev->dev.reset = &FluidMidiReset;
    fdev->dev.generate = &FluidMidiGenerate;
    fdev->dev.sendNoteOff = &FluidMidiSendNoteOff;
    fdev->dev.sendNoteOn = &FluidMidiSendNoteOn;
    fdev->dev.sendNoteAfterTouch = &FluidMidiSendNoteAfterTouch;
    fdev->dev.sendControllerChange = &FluidMidiSendControllerChange;
    fdev->dev.sendProgramChange = &FluidMidiSendProgramChange;
    fdev->dev.sendChannelAfterTouch = &FluidMidiSendChannelAfterTouch;
    fdev->dev.sendPitchBendML = &FluidMidiSendPitchBendML;
    fdev->dev.getOutputCount = &FluidMidiGetOutputCount;
    fdev->dev.getOutputName = &FluidMidiGetOutputName;
    fdev->dev.isOpen = 0;
    fdev->dev.outputIndex = 0;
    fdev->dev.deviceType = Music_FluidSynth;
    fdev->dev.musicType = MUSICTYPE_GENMIDI;
    return &(fdev->dev);
}
#endif // USE_FLUIDSYNTH

//------------------------------------------------------------------------------
MusicDevice *CreateMusicDevice(MusicType type)
{
    MusicDevice *dev = 0;

    switch (type)
    {
    case Music_None:
        dev = createNullMidiDevice();
        break;
    case Music_AdlMidi:
        dev = createAdlMidiDevice();
        break;
    case Music_Native:
        dev = createNativeMidiDevice();
        break;
#ifdef USE_FLUIDSYNTH
    case Music_FluidSynth:
        dev = createFluidSynthDevice();
        break;
#endif
    }

    return dev;
}
