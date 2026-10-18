#define NUM_THREADS 8
#define MASTER_THREAD (NUM_THREADS - 1) // plays the theme's master track; the others play its pieces

#define THREAD_INIT 0
#define THREAD_READY 1
#define THREAD_PLAYTRACK 2
#define THREAD_STOPTRACK 3
#define THREAD_EXIT 4

extern unsigned int NumTracks;

//-1: no thread is using this device channel;  0- : thread index that is using this device channel
extern char ChannelThread[16]; // 16 device channels

extern int NumUsedChannels; // number of in-use device channels

void FreeXMI(void);
int ReadXMI(const char *filename);
void StartTrack(int i, unsigned int track);
void StartTrackLooped(int i, unsigned int track);
void StartMasterClock(void);
int GetTrackNumChannels(unsigned int track);
void XmiSetRequests(const int *pieces, int count);
int XmiConsumeAiDue(void);
void XmiSetPieceMeasures(const unsigned char *measures, int count, int master_measures);
void StopTrack(int i);
void StopTheMusic(void);
int IsPlaying(int i);
void InitReadXMI(void);
void InitDecXMI(void);
void ReloadDecXMI(void);
void ShutdownReadXMI(void);
unsigned int GetOutputCountXMI(void);
void GetOutputNameXMI(const unsigned int outputIndex, char *buffer, const unsigned int bufferSize);
void UpdateVolumeXMI(void);

struct midi_event_struct {
    int time;
    unsigned char status;
    unsigned char data[2];
    unsigned int len;
    unsigned char *buffer;
    struct midi_event_struct *next;
};

typedef struct midi_event_struct MIDI_EVENT;

// XMIDI loop (controllers 116 "FOR" / 117 "NEXT") of a track; start_event is NULL if it has none
typedef struct {
    MIDI_EVENT *start_event;
    MIDI_EVENT *end_event;
    int count; // 0 = forever
} TrackLoop;

extern MIDI_EVENT **TrackEvents;
extern TrackLoop *TrackLoops;
extern short *TrackTiming;
extern unsigned short *TrackUsedChannels;

extern char ThreadChannelRemap[16 * NUM_THREADS];
extern SDL_atomic_t DeviceChannelVolume[16]; // only msb: 0-127
extern SDL_atomic_t ThreadPlaying[NUM_THREADS];
extern SDL_atomic_t ThreadCommand[NUM_THREADS];

struct thread_data {
    int i; // thread index
};
