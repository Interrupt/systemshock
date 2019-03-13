
#include <Carbon/Carbon.h>

// Stub out all of the functions that we haven't implemented yet!

Boolean				gTuneDone;

void StopMovie(void * x) { (unused)x; }
void StartMovie(void * m) { (unused)m; }
Boolean IsMovieDone(void * x) { (unused)x; return true; }

void HideCursor(unused) { }
void SS_ShowCursor(unused) { }
ushort GetOSEvent(short eventMask,EventRecord *theEvent) { (unused)eventMask; (unused)theEvent; return false; }

void AdvanceProgress(unused) { }
void EndProgressDlg(unused) { }

//Boolean ShockAlertFilterProc(DialogPtr dlog, EventRecord *evt, short *itemHit) { return false; }


void MoviesTask(void *m, int n) { (unused)m; (unused)n; }

void DisposHandle(void *h) { (unused)h; }
void DisposeMovie(void *m) { (unused)m; }
void DisposCTable(void *c) { (unused)c; }

void BlitLargeAlign(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus)
{
    (unused)draw_buffer; (unused)dstRowBytes; (unused)dstPtr; (unused)w; (unused)h; (unused)modulus;
}
void BlitLargeAlignSkip(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus)
{
    (unused)draw_buffer; (unused)dstRowBytes; (unused)dstPtr; (unused)w; (unused)h; (unused)modulus;
}
