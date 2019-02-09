#include "MusicDevice.h"
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Dummy MIDI player

static int NullMidiInit(MusicDevice *dev, unsigned samplerate)
{
    return 0;
}

static void NullMidiDestroy(MusicDevice *dev)
{
    free(dev);
}

static void NullMidiSetupMode(MusicDevice *dev, MusicMode mode)
{
}

static void NullMidiReset(MusicDevice *dev)
{
}

static void NullMidiGenerate(MusicDevice *dev, short *samples, int numframes)
{
    memset(samples, 0, numframes * sizeof(short));
}

static void NullMidiSendNoteOff(MusicDevice *dev, int channel, int note, int vel)
{
}

static void NullMidiSendNoteOn(MusicDevice *dev, int channel, int note, int vel)
{
}

static void NullMidiSendNoteAfterTouch(MusicDevice *dev, int channel, int note, int touch)
{
}

static void NullMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val)
{
}

static void NullMidiSendProgramChange(MusicDevice *dev, int channel, int pgm)
{
}

static void NullMidiSendChannelAfterTouch(MusicDevice *dev, int channel, int touch)
{
}

static void NullMidiSendPitchBendML(MusicDevice *dev, int channel, int msb, int lsb)
{
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

static int AdlMidiInit(MusicDevice *dev, unsigned samplerate)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    struct ADL_MIDIPlayer *adl = adl_init(samplerate);

    adl_switchEmulator(adl, ADLMIDI_EMU_NUKED_174);
    adl_setNumChips(adl, 1);
    adl_setVolumeRangeModel(adl, ADLMIDI_VolumeModel_AUTO);
    adl_setRunAtPcmRate(adl, 1);

    adev->adl = adl;
    return 0;
}

static void AdlMidiDestroy(MusicDevice *dev)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_close(adev->adl);
    free(adev);
}

static void AdlMidiSetupMode(MusicDevice *dev, MusicMode mode)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;

    //Use sound bank 45 for res/sound/sblaster, 0 for res/sound/genmidi
    adl_setBank(adev->adl, (mode == Music_SoundBlaster) ? 45 : 0);
}

static void AdlMidiReset(MusicDevice *dev)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_reset(adev->adl);
}

static void AdlMidiGenerate(MusicDevice *dev, short *samples, int numframes)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_generate(adev->adl, 2 * numframes, samples);
}

static void AdlMidiSendNoteOff(MusicDevice *dev, int channel, int note, int vel)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_rt_noteOff(adev->adl, channel, note);
    (void)vel;
}

static void AdlMidiSendNoteOn(MusicDevice *dev, int channel, int note, int vel)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_rt_noteOn(adev->adl, channel, note, vel);
}

static void AdlMidiSendNoteAfterTouch(MusicDevice *dev, int channel, int note, int touch)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_rt_noteAfterTouch(adev->adl, channel, note, touch);
}

static void AdlMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_rt_controllerChange(adev->adl, channel, ctl, val);
}

static void AdlMidiSendProgramChange(MusicDevice *dev, int channel, int pgm)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_rt_patchChange(adev->adl, channel, pgm);
}

static void AdlMidiSendChannelAfterTouch(MusicDevice *dev, int channel, int touch)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_rt_channelAfterTouch(adev->adl, channel, touch);
}

static void AdlMidiSendPitchBendML(MusicDevice *dev, int channel, int msb, int lsb)
{
    AdlMidiDevice *adev = (AdlMidiDevice *)dev;
    adl_rt_pitchBendML(adev->adl, channel, msb, lsb);
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
    return &adev->dev;
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

static int FluidMidiInit(MusicDevice *dev, unsigned samplerate)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    int sfid;

    settings = new_fluid_settings();
    fluid_settings_setnum(settings, "synth.sample-rate", samplerate);

    synth = new_fluid_synth(settings);
    sfid = fluid_synth_sfload(synth, "res/music.sf2", 1);

    if (sfid == FLUID_FAILED)
    {
        WARN("cannot load res/music.sf2 for FluidSynth");
        delete_fluid_synth(synth);
        delete_fluid_settings(settings);
        fdev->synth = NULL;
        fdev->settings = NULL;
        return -1;
    }

    fluid_synth_sfont_select(synth, 0, sfid);

    fdev->synth = synth;
    fdev->settings = settings;

    return 0;
}

static void FluidMidiDestroy(MusicDevice *dev)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    delete_fluid_synth(fdev->synth);
    delete_fluid_settings(fdev->settings);
    free(dev);
}

static void FluidMidiSetupMode(MusicDevice *dev, MusicMode mode)
{
    (void)dev;
    (void)mode;
}

static void FluidMidiReset(MusicDevice *dev)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_system_reset(synth);
}

static void FluidMidiGenerate(MusicDevice *dev, short *samples, int numframes)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_write_s16(synth, numframes,
                          samples, 0, 2, /* left channel*/
                          samples, 1, 2  /* right channel*/);
}

static void FluidMidiSendNoteOff(MusicDevice *dev, int channel, int note, int vel)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_noteoff(synth, channel, note);
    (void)vel;
}

static void FluidMidiSendNoteOn(MusicDevice *dev, int channel, int note, int vel)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_noteon(synth, channel, note, vel);
}

static void FluidMidiSendNoteAfterTouch(MusicDevice *dev, int channel, int note, int touch)
{
#if FLUIDSYNTH_VERSION_MAJOR >= 2
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_key_pressure(synth, channel, note, touch);
#else
    (void)dev;
    (void)channel;
    (void)note;
    (void)touch;
#endif
}

static void FluidMidiSendControllerChange(MusicDevice *dev, int channel, int ctl, int val)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_cc(synth, channel, ctl, val);
}

static void FluidMidiSendProgramChange(MusicDevice *dev, int channel, int pgm)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_program_change(synth, channel, pgm);
}

static void FluidMidiSendChannelAfterTouch(MusicDevice *dev, int channel, int touch)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_channel_pressure(synth, channel, touch);
}

static void FluidMidiSendPitchBendML(MusicDevice *dev, int channel, int msb, int lsb)
{
    FluidMidiDevice *fdev = (FluidMidiDevice *)dev;
    fluid_synth_t *synth = fdev->synth;
    fluid_synth_pitch_bend(synth, channel, msb * 128 + lsb);
}

static MusicDevice *createFluidSynthDevice()
{
    FluidMidiDevice *adev = malloc(sizeof(FluidMidiDevice));
    adev->dev.init = &FluidMidiInit;
    adev->dev.destroy = &FluidMidiDestroy;
    adev->dev.setupMode = &FluidMidiSetupMode;
    adev->dev.reset = &FluidMidiReset;
    adev->dev.generate = &FluidMidiGenerate;
    adev->dev.sendNoteOff = &FluidMidiSendNoteOff;
    adev->dev.sendNoteOn = &FluidMidiSendNoteOn;
    adev->dev.sendNoteAfterTouch = &FluidMidiSendNoteAfterTouch;
    adev->dev.sendControllerChange = &FluidMidiSendControllerChange;
    adev->dev.sendProgramChange = &FluidMidiSendProgramChange;
    adev->dev.sendChannelAfterTouch = &FluidMidiSendChannelAfterTouch;
    adev->dev.sendPitchBendML = &FluidMidiSendPitchBendML;
    return &adev->dev;
}
#endif // USE_FLUIDSYNTH

//------------------------------------------------------------------------------
MusicDevice *CreateMusicDevice(MusicType type)
{
    MusicDevice *dev;

    switch (type)
    {
    default:
        break;
    case Music_AdlMidi:
        dev = createAdlMidiDevice();
        break;
#ifdef USE_FLUIDSYNTH
    case Music_FluidSynth:
        dev = createFluidSynthDevice();
        break;
#endif
    }

    return dev;
}
