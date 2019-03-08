#define NUM_THREADS  8

#define THREAD_INIT       0
#define THREAD_READY      1
#define THREAD_PLAYTRACK  2
#define THREAD_STOPTRACK  3
#define THREAD_EXIT       4

unsigned int NumTracks;

//-1: no thread is using this device channel;  0- : thread index that is using this device channel
char ChannelThread[16]; //16 device channels

int NumUsedChannels; //number of in-use device channels

void FreeXMI(void);
int ReadXMI(const char *filename);
void StartTrack(int i, unsigned int track, int volume);
void StopTrack(int i);
void StopTheMusic(void);
int IsPlaying(int i);
void InitReadXMI(void);
void InitDecXMI(void);
void ReloadDecXMI(void);
void ShutdownReadXMI(void);

struct midi_event_struct
{
  int time;
  unsigned char status;
  unsigned char data[2];
  unsigned int len;
  unsigned char *buffer;
  struct midi_event_struct *next;
};

typedef struct midi_event_struct  MIDI_EVENT;

MIDI_EVENT **TrackEvents;
short *TrackTiming;
unsigned short *TrackUsedChannels;

MIDI_EVENT *ThreadEventList[NUM_THREADS];
int ThreadTiming[NUM_THREADS];
char ThreadChannelRemap[16*NUM_THREADS];
SDL_atomic_t ThreadVolume[NUM_THREADS]; //only msb: 0-127
SDL_atomic_t ThreadPlaying[NUM_THREADS];
SDL_atomic_t ThreadCommand[NUM_THREADS];

struct thread_data
{
  int i; //thread index
};
