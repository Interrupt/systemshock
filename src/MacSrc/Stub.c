
#include <Carbon/Carbon.h>

// Stub out all of the functions that we haven't implemented yet!


// Start with the sound library functions
typedef struct {

} snd_digi_parms;

Boolean				gTuneDone;

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
Boolean IsMovieDone(void) { return true; }

void HideCursor(void) { }
void ShowCursor(void) { }

void AdvanceProgress(void) { }
void EndProgressDlg(void) { }