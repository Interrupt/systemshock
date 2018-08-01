#include "anim.h"
#include "tools.h"
#include "faketime.h"
#include "gr2ss.h"
#include <SDL.h>

ActAnim current_anim;

long next_time = 0;
bool first_frame = TRUE;

extern void SDLDraw(void);
extern void pump_events(void);

// play
void AnimRecur() {
	// advance to next frame
	// DEBUG("Playing frame %x!", current_anim.currFrameRef);

	int x = SCONV_X(current_anim.reg->abs_x);
	int y = SCONV_Y(current_anim.reg->abs_y);

	x = 0;
	y = 0;

	AnimCodeData *data = &current_anim.pah->data[current_anim.curSeq];
	grs_bitmap unpackBM;

	DEBUG("%i", data->frameDelay);

	if(first_frame == TRUE) {
		if(current_anim.composeFunc != NULL)
			current_anim.composeFunc(current_anim.reg->r, 0x04);

		first_frame = FALSE;
	}

	short a, b, c, d;
	STORE_CLIP(a, b, c, d);

	FrameDesc *f = (FrameDesc *)RefLock(current_anim.currFrameRef);
    if (f != NULL) {
    	f->bm.bits = (uchar *)(f + 1);
    	f->bm.flags = BMF_TRANS;

    	gr_set_cliprect(f->updateArea.ul.x, f->updateArea.ul.y, f->updateArea.lr.x, f->updateArea.lr.y);
    	gr_rsd8_bitmap((grs_bitmap *)f, x, y);
    }
    else {
    	DEBUG("Done playing anim!");
		current_anim.notifyFunc(&current_anim, ANCODE_KILL, data);
    }
    RefUnlock(current_anim.currFrameRef);

    long time = SDL_GetTicks();
	if(time >= next_time) {
		current_anim.currFrameRef++;
		current_anim.frameNum++;
		next_time = time + data->frameDelay;

		if(current_anim.frameNum > data->frameRunEnd + 1) {
			current_anim.curSeq++;
		}
	}

	RESTORE_CLIP(a, b, c, d);

	// Make SDL happy
	pump_events();
	SDLDraw();
}

void AnimSetAnimPall(Ref animRef) {

}

bool AnimPreloadFrames(ActAnim *paa, Ref animRef) {
	return 1;
}

ActAnim *AnimPlayRegion(Ref animRef, LGRegion *region, Point loc, char unknown,
   void (*composeFunc)(Rect *area, ubyte flags)) {
	// start playing

	DEBUG("Playing animation: %x", animRef);

	AnimHead *head = (AnimHead *)RefGet(animRef);
	if(head != NULL) {
		DEBUG("Animation frames at %x", head->frameSetId);
	}

	first_frame = TRUE;
	current_anim.reg = region;
	current_anim.pah = head;
	current_anim.currFrameRef = MKREF(head->frameSetId, 0);
	current_anim.startFrameRef = current_anim.currFrameRef;
	current_anim.curSeq = 0;
	current_anim.frameNum = 0;
	current_anim.composeFunc = composeFunc;

	next_time = SDL_GetTicks() + current_anim.pah->data[0].frameDelay;

	return &current_anim;
}

void AnimSetNotify(ActAnim *paa, void *powner, AnimCode mask,
        void (*func) (ActAnim *, AnimCode ancode, AnimCodeData *animData)) {

	paa->notifyFunc = func;
	// user callback function
}

void AnimKill(ActAnim *paa) {
	// Stop animation
	DEBUG("Anim killed.");
    AnimCodeData data;
	current_anim.notifyFunc(&current_anim, ANCODE_KILL, &data);
}
