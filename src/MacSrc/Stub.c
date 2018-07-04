
#include <Carbon/Carbon.h>

// Stub out all of the functions that we haven't implemented yet!

Boolean				gTuneDone;

void StopMovie(void * x) { }
void StartMovie(void * m) { }
Boolean IsMovieDone(void * x) { return true; }

void HideCursor(void) { }
void ShowCursor(void) { }
ushort GetOSEvent(short eventMask,EventRecord *theEvent) { return false; }

void AdvanceProgress(void) { }
void EndProgressDlg(void) { }

//Boolean ShockAlertFilterProc(DialogPtr dlog, EventRecord *evt, short *itemHit) { return false; }


void MoviesTask(void *m, int n) { }

void DisposHandle(void *h) { }
void DisposeMovie(void *m) { }
void DisposCTable(void *c) { }

void BlitLargeAlign(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus) { }
void BlitLargeAlignSkip(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus) { }
