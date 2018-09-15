#include "Xmi.h"
#include <SDL.h>
#include "adlmidi.h"



struct ADL_MIDIPlayer *adlD;

SDL_mutex *MyMutex;



extern SDL_AudioStream *cutscene_audiostream;



static void AdlAudioCallback(void *d, unsigned char *stream, int len)
{
  SDL_AudioStream *as = *(SDL_AudioStream **)d;

  if (as != NULL) SDL_AudioStreamGet(as, stream, len);
  else
  {
    SDL_LockMutex(MyMutex);
    adl_generate(adlD, len / 2, (short *)stream);
    SDL_UnlockMutex(MyMutex);
  }
}



void FreeXMI(void)
{
  unsigned int track;
  MIDI_EVENT *event, *next;

  for (track=0; track<NumTracks; track++)
  {
    event = TrackEvents[track];
    while (event)
    {
      next = event->next;
      if (event->buffer) free(event->buffer);
      free(event);
      event = next;
    }
  }

  if (TrackEvents) {free(TrackEvents); TrackEvents = 0;}
  if (TrackTiming) {free(TrackTiming); TrackTiming = 0;}
  if (TrackUsedChannels) {free(TrackUsedChannels); TrackUsedChannels = 0;}

  NumTracks = 0;
}



MIDI_EVENT *NewMIDIEvent(MIDI_EVENT **eventlist, MIDI_EVENT *curevent, int time)
{
  if (*eventlist == 0)
  {
    *eventlist = curevent = (MIDI_EVENT *)malloc(sizeof(MIDI_EVENT));

    curevent->next = 0;

    if (time < 0) curevent->time = 0; else curevent->time = time;
    curevent->buffer = 0;
    curevent->len = 0;

    return curevent;
  }

  if (time < 0)
  {
    MIDI_EVENT *event = (MIDI_EVENT *)malloc(sizeof(MIDI_EVENT));

    event->next = *eventlist;
    *eventlist = curevent = event;

    curevent->time = 0;
    curevent->buffer = 0;
    curevent->len = 0;

    return curevent;
  }

  if (curevent->time > time) curevent = *eventlist;

  while (curevent->next)
  {
    if (curevent->next->time > time)
    {
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



int ReadXMI(const char *filename)
{
  FILE *f;
  int size, start, begin, pos, time, end, tempo, tempo_set;
  unsigned int i, count, len, chunk_len, quant, status, delta, b0, b1, b2, b3;
  unsigned char *data, *p;
  short ppqn;
  unsigned short used_channels;
  MIDI_EVENT *eventlist, *curevent, *prev;
  char buf[32];

  INFO("Reading XMI %s", filename);

  f = fopen(filename, "rb");
  if (f == 0) {
  	ERROR("Could not read XMI");
  	return 0;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
  data = (unsigned char *)malloc(size);
  if (fread(data, size, 1, f) != 1) {free(data); fclose(f); return 0;}
  fclose(f);

  p = data;

  memcpy(buf, p, 4); p += 4;
  if (memcmp(buf, "FORM", 4)) {free(data); return 0;} //is not an xmi

  b3 = *p++; b2 = *p++; b1 = *p++; b0 = *p++;
  len = b0 | (b1<<8) | (b2<<16) | (b3<<24);

  start = p - data;

  memcpy(buf, p, 4); p += 4;
  if (!memcmp(buf, "XMID", 4)) NumTracks = 1; //XMI doesn't have XDIR, so there's only one track
  else if (memcmp(buf, "XDIR", 4)) {free(data); return 0;} //invalid XMI format
  else
  {
    NumTracks = 0;

    for (i=4; i<len; i++)
    {
      memcpy(buf, p, 4); p += 4;

      b3 = *p++; b2 = *p++; b1 = *p++; b0 = *p++;
      chunk_len = b0 | (b1<<8) | (b2<<16) | (b3<<24);

      i += 8;

      if (memcmp(buf, "INFO", 4))
      {
        p += ((chunk_len+1) & ~1);
        i += ((chunk_len+1) & ~1);
        continue;
      }

      if (chunk_len < 2) break;

      b0 = *p++; b1 = *p++;
      NumTracks = b0 | (b1 << 8);

      break;
    }

    if (NumTracks == 0) {free(data); return 0;} //xmi must have at least one track

    p = data + start + ((len+1) & ~1);

    memcpy(buf, p, 4); p += 4;
    if (memcmp(buf, "CAT ", 4)) {free(data); return 0;} //invalid XMI format

    b3 = *p++; b2 = *p++; b1 = *p++; b0 = *p++;
    len = b0 | (b1<<8) | (b2<<16) | (b3<<24);

    memcpy(buf, p, 4); p += 4;
    if (memcmp(buf, "XMID", 4)) {free(data); return 0;} //invalid XMI format
  }

  INFO("NumTracks: %i", NumTracks);

  TrackEvents = (MIDI_EVENT **)malloc(NumTracks * sizeof(MIDI_EVENT *));
  TrackTiming = (short *)malloc(NumTracks * sizeof(short));
  TrackUsedChannels = (unsigned short *)malloc(NumTracks * sizeof(unsigned short));


  for (i=0; i<NumTracks; i++) TrackEvents[i] = 0;

  count = 0;

  while (p - data < size && count != NumTracks)
  {
    memcpy(buf, p, 4); p += 4;

    b3 = *p++; b2 = *p++; b1 = *p++; b0 = *p++;
    len = b0 | (b1<<8) | (b2<<16) | (b3<<24);

    if (!memcmp(buf, "FORM", 4))
    {
      p += 4;

      memcpy(buf, p, 4); p += 4;

      b3 = *p++; b2 = *p++; b1 = *p++; b0 = *p++;
      len = b0 | (b1<<8) | (b2<<16) | (b3<<24);
    }

    if (memcmp(buf, "EVNT", 4))
    {
      p += ((len+1) & ~1);
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
  
    while (!end && p - data < size)
    {
      quant = 0; for (i=0; i<4; i++) {b0 = *p++; if (b0 & 0x80) {p--; break;} quant += b0;}
      time += quant*3;
  
      status = *p++;
      switch (status >> 4)
      {
        case 0x9: //note on
          used_channels |= (1 << (status & 15));
          b0 = *p++;
          curevent = NewMIDIEvent(&eventlist, curevent, time);
          curevent->status = status;
          curevent->data[0] = b0;
          curevent->data[1] = *p++;
          delta = 0; for (i=0; i<4; i++) {b1 = *p++; delta <<= 7; delta |= b1 & 0x7F; if (!(b1 & 0x80)) break;}
          prev = curevent;
          curevent = NewMIDIEvent(&eventlist, curevent, time + delta*3);
          curevent->status = status;
          curevent->data[0] = b0;
          curevent->data[1] = 0;
          curevent = prev;
        break;
  
        case 0x8: case 0xA: case 0xB: case 0xE: //note off, aftertouch, controller, pitch wheel
          used_channels |= (1 << (status & 15));
          curevent = NewMIDIEvent(&eventlist, curevent, time);
          curevent->status = status;
          curevent->data[0] = *p++;
          curevent->data[1] = *p++;
        break;
  
        case 0xC: case 0xD: //program change, pressure
          used_channels |= (1 << (status & 15));
          curevent = NewMIDIEvent(&eventlist, curevent, time);
          curevent->status = status;
          curevent->data[0] = *p++;
        break;
 
        case 0xF: //sysex
          if (status == 0xFF)
          {
            pos = p - data;
            b0 = *p++;
            if (b0 == 0x2F) end = 1;
            else if (b0 == 0x51 && !tempo_set)
            {
              p++; b3 = *p++; b2 = *p++; b1 = *p++;
              tempo = (b1 | (b2 << 8) | (b3 << 16)) * 3;
              tempo_set = 1;
            }
            else if (b0 == 0x51 && tempo_set)
            {
              quant = 0; for (i=0; i<4; i++) {b1 = *p++; quant <<= 7; quant |= b1 & 0x7F; if (!(b1 & 0x80)) break;}
              p += quant;
              break;
            }
            p = data + pos;
          }
          curevent = NewMIDIEvent(&eventlist, curevent, time);
          curevent->status = status;
          if (status == 0xFF) curevent->data[0] = *p++;
          quant = 0; for (i=0; i<4; i++) {b0 = *p++; quant <<= 7; quant |= b0 & 0x7F; if (!(b0 & 0x80)) break;}
          curevent->len = quant;
          if (curevent->len)
          {
            curevent->buffer = (unsigned char *)malloc(curevent->len);
            memcpy(curevent->buffer, p, curevent->len);
            p += curevent->len;
          }
        break;
  
        default: break;
      }
    }
  
    ppqn = (tempo * 3) / 25000;
    if (!ppqn) break; //unable to convert data

    TrackEvents[count] = eventlist;
    TrackTiming[count] = ppqn;
    TrackUsedChannels[count] = used_channels;
    count++;

    p = data + begin + ((len+1) & ~1);
  }

  if (count != NumTracks) //failed to extract all tracks from XMI
  {
    free(data);
    NumTracks = count;
    FreeXMI();
    return 0;
  }


  free(data);


  //Use sound bank 45 for res/sound/sblaster, 0 for res/sound/genmidi
  SDL_LockMutex(MyMutex);
  if (strstr(filename, "sblaster") != NULL) adl_setBank(adlD, 45);
  else                                      adl_setBank(adlD, 0);
  SDL_UnlockMutex(MyMutex);

  return 1; //success
}



int MyThread(void *arg)
{
  int i = ((struct thread_data *)arg)->i; //thread index passed by arg
  MIDI_EVENT *event = 0, *evntlist = 0;
  int ppqn = 1, tempo = 0x07A120, channel;
  unsigned int start = 0;
  double Ippqn = 1, tick = 1, last_tick = 0, last_time = 0, aim = 0, diff = 0;


  SDL_AtomicSet(&ThreadPlaying[i], 0);
  SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);


  for (;;)
  {
    while (event && SDL_AtomicGet(&ThreadCommand[i]) != THREAD_STOPTRACK)
    {
      aim = last_time + (event->time - last_tick) * tick;
      diff = aim - ((SDL_GetTicks() - start) * 1000.0);
      if (diff > 0) break;
      last_tick = event->time;
      last_time = aim;

      if (event->status == 0xFF && event->data[0] == 0x51) //tempo change
      {
        tempo = (event->buffer[0] << 16) | (event->buffer[1] << 8) | event->buffer[2];
        tick = tempo * Ippqn;
      }
      else if (event->status >= 0x80 && event->status < 0xF0)
      {
        channel = (event->status & 15);
        if (channel != 9) channel = ThreadChannelRemap[channel+16*i]; //remap channel, except 9 (percussion)

        uint8_t p1 = event->data[0];
        uint8_t p2 = event->data[1];

        if ((event->status & ~15) == 0xB0 && event->data[0] == 0x07)
        {
          //set volume msb
          SDL_LockMutex(MyMutex);
          adl_rt_controllerChange(adlD, channel, p1, (int)SDL_AtomicGet(&ThreadVolume[i]) * p2 / 128);
          SDL_UnlockMutex(MyMutex);
    	}
    	else
        {
          SDL_LockMutex(MyMutex);
          switch (event->status & ~15)
          {
            case 0x80: adl_rt_noteOff(adlD, channel, p1); break;
            case 0x90: adl_rt_noteOn(adlD, channel, p1, p2); break;
            case 0xA0: adl_rt_noteAfterTouch(adlD, channel, p1, p2); break;
            case 0xB0: adl_rt_controllerChange(adlD, channel, p1, p2); break;
            case 0xC0: adl_rt_patchChange(adlD, channel, p1); break;
            case 0xD0: adl_rt_channelAfterTouch(adlD, channel, p1); break;
            case 0xE0: adl_rt_pitchBendML(adlD, channel, p2, p1); break;
          }
          SDL_UnlockMutex(MyMutex);
        }
      }

      event = event->next;
      if (event == 0) SDL_AtomicSet(&ThreadCommand[i], THREAD_STOPTRACK);
    }


    if (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_STOPTRACK)
    {
      event = evntlist = 0;

      start = SDL_GetTicks();
      last_tick = 0;
      last_time = 0;

      //here we should turn off all notes for these channels plus 9 (percussion)
      for (channel=0; channel<16; channel++) if (ChannelThread[channel] == i)
      {
        ChannelThread[channel] = -1;
        NumUsedChannels--;
      }

      SDL_AtomicSet(&ThreadPlaying[i], 0);
      SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
    }


    if (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_PLAYTRACK)
    {
      event = evntlist = ThreadEventList[i];
      ppqn = ThreadTiming[i];

      tempo = 0x07A120;
      Ippqn = 1.0 / ppqn;
      tick = tempo * Ippqn;

      start = SDL_GetTicks();
      last_tick = 0;
      last_time = 0;

      SDL_AtomicSet(&ThreadPlaying[i], 1);
      SDL_AtomicSet(&ThreadCommand[i], THREAD_READY);
    }


    if (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_EXIT) break;


    if (event)
    {
      aim = last_time + (event->time - last_tick) * tick;
      diff = aim - ((SDL_GetTicks() - start) * 1000.0);
    }
    else diff = 1000;


    if (diff >= 1000) SDL_Delay(1);
    else if (diff >= 200) SDL_Delay(0);
  }

  return 0;
}



int GetTrackNumChannels(unsigned int track)
{
  int num = 0, channel;

  //count channels used by track (could be zero if only percussion channel (9) is used)
  for (channel=0; channel<16; channel++) if (channel != 9 && (TrackUsedChannels[track] & (1 << channel))) num++;

  return num;
}



void StartTrack(int i, unsigned int track, int volume)
{
  int num, channel, remap;
  char channel_remap[16];

  if (track >= NumTracks) return;

  num = GetTrackNumChannels(track);


  while (SDL_AtomicGet(&ThreadCommand[i]) != THREAD_READY) SDL_Delay(1);


  //check if enough device channels free; 16 channels available except one (percussion)
  if (NumUsedChannels + num <= 16-1)
  {
    NumUsedChannels += num;

    memset(channel_remap, 0, 16);

    //assign channels used by track to device channels that are currently free
    for (channel=0; channel<16; channel++) if (channel != 9 && (TrackUsedChannels[track] & (1 << channel)))
    {
      for (remap=0; remap<16; remap++) if (remap != 9 && ChannelThread[remap] == -1) break;
      channel_remap[channel] = remap;
      ChannelThread[remap] = i;
    }

    ThreadEventList[i] = TrackEvents[track];
    ThreadTiming[i] = TrackTiming[track];
    memcpy(ThreadChannelRemap+16*i, channel_remap, 16);

    SDL_AtomicSet(&ThreadVolume[i], volume);

    SDL_AtomicSet(&ThreadCommand[i], THREAD_PLAYTRACK);
  
    while (SDL_AtomicGet(&ThreadCommand[i]) != THREAD_READY) SDL_Delay(1);
  }


  SDL_PauseAudio(0);
}



void StopTrack(int i)
{
  if (!SDL_AtomicGet(&ThreadPlaying[i])) return;

  while (SDL_AtomicGet(&ThreadCommand[i]) != THREAD_READY) SDL_Delay(1);

  SDL_AtomicSet(&ThreadCommand[i], THREAD_STOPTRACK);

  while (SDL_AtomicGet(&ThreadCommand[i]) != THREAD_READY) SDL_Delay(1);
}



void StopTheMusic(void)
{
  int i;

  for (i=0; i<NUM_THREADS; i++) StopTrack(i);

  SDL_PauseAudio(1);
  SDL_Delay(1);

  SDL_LockMutex(MyMutex);
  adl_reset(adlD);
  SDL_UnlockMutex(MyMutex);
}



int IsPlaying(int i)
{
  return SDL_AtomicGet(&ThreadPlaying[i]);
}



void ForceMusicVolume(int volume)
{
  int i;

  //force all playing channels to volume
  for (i=0; i<NUM_THREADS; i++) if (IsPlaying(i))
  {
    SDL_AtomicSet(&ThreadVolume[i], volume);
  
    SDL_LockMutex(MyMutex);
    adl_rt_controllerChange(adlD, i, 0x07, volume); //0-127 msb
    SDL_UnlockMutex(MyMutex);
  }
}



void InitReadXMI(void)
{
  int i, channel;


  // Start the ADL Midi device
  adlD = adl_init(48000);

  adl_switchEmulator(adlD, ADLMIDI_EMU_NUKED_174);
  adl_setNumChips(adlD, 1);
  adl_setVolumeRangeModel(adlD, ADLMIDI_VolumeModel_AUTO);
  adl_setRunAtPcmRate(adlD, TRUE);



  SDL_AudioSpec spec, obtained;
  spec.freq     = 48000;
  spec.format   = AUDIO_S16SYS;
  spec.channels = 2;
  spec.samples  = 2048;
  spec.callback = AdlAudioCallback;
  spec.userdata = (void *)&cutscene_audiostream;

  if (SDL_OpenAudio(&spec, &obtained)) {
  	ERROR("Could not open SDL audio");
  }
  else {
  	INFO("Opened Music Stream");
  }


  if (Mix_Init(MIX_INIT_MP3) < 0) {
    ERROR("%s: Init failed", __FUNCTION__);
  }

  if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
    ERROR("%s: Couldn't open audio device", __FUNCTION__);
  }

  Mix_AllocateChannels(SND_MAX_SAMPLES);


  for (channel=0; channel<16; channel++) ChannelThread[channel] = -1;


  MyMutex = SDL_CreateMutex();


  for (i=0; i<NUM_THREADS; i++)
  {
    //data is argument to MyThread function; it must "outlive" thread, so it is malloc'ed
    //data->i tells thread function which thread index to use
    struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));
    SDL_Thread *thread;

    SDL_AtomicSet(&ThreadPlaying[i], 0);
    SDL_AtomicSet(&ThreadCommand[i], THREAD_INIT);

    data->i = i;
    thread = SDL_CreateThread(MyThread, "MyThread", (void *)data);
    SDL_DetachThread(thread); //thread will go away on its own upon completion
  
    while (SDL_AtomicGet(&ThreadCommand[i]) == THREAD_INIT) SDL_Delay(1);
  }


  atexit(ShutdownReadXMI);
}



void ShutdownReadXMI(void)
{
  int i;

  for (i=0; i<NUM_THREADS; i++)
  {
    StopTrack(i);
    SDL_AtomicSet(&ThreadCommand[i], THREAD_EXIT);
  }

  SDL_Delay(50); //wait a bit for the threads to all hopefully exit

  FreeXMI();

  Mix_CloseAudio();

  SDL_CloseAudio();

  adl_close(adlD);

  SDL_DestroyMutex(MyMutex);
}
