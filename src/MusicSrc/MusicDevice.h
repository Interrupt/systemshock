#pragma once

typedef struct MusicDevice MusicDevice;

typedef enum MusicType
{
    Music_None
   ,Music_AdlMidi
   ,Music_Native
#ifdef USE_FLUIDSYNTH
   ,Music_FluidSynth
#endif
} MusicType;

typedef enum MusicMode
{
    Music_GeneralMidi,
    Music_SoundBlaster,
} MusicMode;

struct MusicDevice
{
    int  (*init)(MusicDevice *dev, const unsigned int outputIndex, unsigned samplerate);
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
    unsigned int (*getOutputCount)(MusicDevice *dev);
    void (*getOutputName)(MusicDevice *dev, const unsigned int outputIndex, char *buffer, const unsigned int bufferSize);
    unsigned short isOpen;    // 1 if device open, 0 if closed
    unsigned int outputIndex; // index of currently opened output
    MusicType deviceType;     // type of device
    char *musicType;          // "sblaster" or "genmidi"
};

#define MUSICTYPE_SBLASTER "sblaster"
#define MUSICTYPE_GENMIDI  "genmidi"

MusicDevice *CreateMusicDevice(MusicType type);
