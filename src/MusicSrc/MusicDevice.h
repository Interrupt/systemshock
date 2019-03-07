#pragma once

typedef struct MusicDevice MusicDevice;

typedef enum MusicType
{
    Music_None,
    Music_AdlMidi,
    Music_Native,
    Music_FluidSynth
} MusicType;

typedef enum MusicMode
{
    Music_GeneralMidi,
    Music_SoundBlaster,
} MusicMode;

struct MusicDevice
{
    int (*init)(MusicDevice *dev, unsigned samplerate);
    void (*destroy)(MusicDevice *dev);

    void (*setupMode)(MusicDevice *dev, MusicMode mode);

    void (*reset)(MusicDevice *dev);
    void (*generate)(MusicDevice *dev, short *samples, int numframes);

    void (*sendNoteOff)(MusicDevice *dev, int channel, int note, int vel);
    void (*sendNoteOn)(MusicDevice *dev, int channel, int note, int vel);
    void (*sendNoteAfterTouch)(MusicDevice *dev, int channel, int note, int touch);
    void (*sendControllerChange)(MusicDevice *dev, int channel, int ctl, int val);
    void (*sendProgramChange)(MusicDevice *dev, int channel, int pgm);
    void (*sendChannelAfterTouch)(MusicDevice *dev, int channel, int touch);
    void (*sendPitchBendML)(MusicDevice *dev, int channel, int msb, int lsb);
};

MusicDevice *CreateMusicDevice(MusicType type);
