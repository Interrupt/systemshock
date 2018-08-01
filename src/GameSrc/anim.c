#include "anim.h"
#include "tools.h"
#include "faketime.h"
#include "gr2ss.h"

ActAnim current_anim;

long next_time = 0;

extern void SDLDraw(void);
extern void pump_events(void);

bool first = TRUE;

// play
void AnimRecur() {
	// advance to next frame
	// DEBUG("Playing frame %x!", current_anim.currFrameRef);

	int x = SCONV_X(current_anim.reg->abs_x);
	int y = SCONV_Y(current_anim.reg->abs_y);

	AnimCodeData *data = &current_anim.pah->data[current_anim.curSeq];
	grs_bitmap unpackBM;

	if(first == TRUE) {
		if(current_anim.composeFunc != NULL)
			current_anim.composeFunc(current_anim.reg->r, 0x04);

		first = FALSE;
	}

	short a, b, c, d;
	STORE_CLIP(a, b, c, d);

	FrameDesc *f = (FrameDesc *)RefLock(current_anim.currFrameRef);
    if (f != NULL) {
    	f->bm.bits = (uchar *)(f + 1);

    	gr_rsd8_convert((grs_bitmap *)f, &unpackBM);
    	unpackBM.flags = BMF_TRANS;

    	// clip the draw area?
    	ss_safe_set_cliprect(f->updateArea.ul.x + x, f->updateArea.ul.y + y, f->updateArea.lr.x + x, f->updateArea.lr.y + y);
    	ss_bitmap(&unpackBM, x, y);
    }
    else {
    	DEBUG("Done playing anim!");
		current_anim.notifyFunc(&current_anim, ANCODE_KILL, data);
    }
    RefUnlock(current_anim.currFrameRef);

	next_time++;
	if(next_time > 5) {
		current_anim.currFrameRef++;
		current_anim.frameNum++;
		next_time = 0;

		if(current_anim.frameNum > data->frameRunEnd) {
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

	current_anim.reg = region;
	current_anim.pah = head;
	current_anim.currFrameRef = MKREF(head->frameSetId, 0);
	current_anim.startFrameRef = current_anim.currFrameRef;
	current_anim.curSeq = 0;
	current_anim.frameNum = 0;
	current_anim.composeFunc = composeFunc;

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
