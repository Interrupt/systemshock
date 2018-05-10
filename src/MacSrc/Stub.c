
#include <Carbon/Carbon.h>

#include <stdbool.h>

#include "lg_types.h"
// Stub out all of the functions that we haven't implemented yet!


// Start with the sound library functions
typedef struct {

} snd_digi_parms;

bool				gTuneDone;

void snd_kill_all_samples(void) { }
void snd_sample_reload_parms(snd_digi_parms *sdp) { }
void snd_end_sample(int hnd_id) { }
void snd_startup(void) { }

int snd_start_digital(void) { }
int snd_stop_digital(void) { }

snd_digi_parms *snd_sample_parms(int hnd_id)
{
	snd_digi_parms parms;
	return &parms;
}

void StopMovie(void) { }
void StartMovie(void * m) { }
bool IsMovieDone(void) { return true; }

void HideCursor(void) { }
void ShowCursor(void) { }
ushort GetOSEvent(short eventMask,EventRecord *theEvent) { return false; }

void AdvanceProgress(void) { }
void EndProgressDlg(void) { }

bool ShockAlertFilterProc(DialogPtr dlog, EventRecord *evt, short *itemHit) { return false; }


MoviesTask(void *m, int n) { }

DisposHandle(void *h) { }
DisposeMovie(void *m) { }
DisposCTable(void *c) { }

void BlitLargeAlign(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus) { }
void BlitLargeAlignSkip(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus) { }