#include "anim.h"
#include "tools.h"
#include "faketime.h"
#include "gr2ss.h"
#include <SDL.h>

ActAnim current_anim;

extern void SDLDraw(void);
extern void pump_events(void);

bool done_playing_anim = false;

// play
void AnimRecur() {
	int x, y = 0;

	if(done_playing_anim)
		return;

	AnimCodeData *data = &current_anim.pah->data[current_anim.curSeq];
	grs_bitmap unpackBM;

	// stop drawing the mouse for now, switch to video canvas
	uiHideMouse(NULL);
	gr_push_canvas(&current_anim.cnv);

	// might need to draw a background before the first frame
	if(current_anim.frameNum == 0) {
		if(current_anim.composeFunc != NULL)
			current_anim.composeFunc(current_anim.reg->r, 0x04);
	}

	short a, b, c, d;
	STORE_CLIP(a, b, c, d);

	// grab this frame
	FrameDesc *f = (FrameDesc *)RefLock(current_anim.currFrameRef);

    if (f != NULL) {
    	f->bm.bits = (uchar *)(f + 1);
    	f->bm.flags = BMF_TRANS;

		gr_set_cliprect(f->updateArea.ul.x, f->updateArea.ul.y, f->updateArea.lr.x, f->updateArea.lr.y);
    	gr_rsd8_bitmap((grs_bitmap *)f, 0, 0);
    }
    else {
    	TRACE("Done playing anim!");
    	done_playing_anim = true;
    }

    RefUnlock(current_anim.currFrameRef);

    RESTORE_CLIP(a, b, c, d);
    gr_pop_canvas();

    // Draw the scaled up movie
    x = SCONV_X(current_anim.reg->abs_x);
	y = SCONV_Y(current_anim.reg->abs_y);
    ss_bitmap(&current_anim.cnv.bm, x, y);

    long time = SDL_GetTicks();
	if(time >= current_anim.timeContinue) {
		current_anim.currFrameRef++;
		current_anim.frameNum++;
		current_anim.timeContinue = time + 100;

		if(current_anim.frameNum > data->frameRunEnd + 1) {
			current_anim.curSeq++;
		}
	}

	// safe to draw the mouse again
	uiShowMouse(NULL);

	if(done_playing_anim) {
		AnimKill(&current_anim);
	}

	// Make SDL happy
	pump_events();
	SDLDraw();
}

void AnimSetAnimPall(Ref animRef) {

}

bool AnimPreloadFrames(ActAnim *paa, Ref animRef) {
	return 1;
}

ActAnim *AnimPlayRegion(Ref animRef, LGRegion *region, LGPoint loc, char unknown,
   void (*composeFunc)(Rect *area, ubyte flags)) {
	// start playing
	DEBUG("Playing animation: %x", animRef);

	AnimHead *head = (AnimHead *)RefGet(animRef);
	if(head != NULL) {
		TRACE("Animation frames at %x", head->frameSetId);
	}

	done_playing_anim = false;
	current_anim.reg = region;
	current_anim.pah = head;
	current_anim.currFrameRef = MKREF(head->frameSetId, 0);
	current_anim.curSeq = 0;
	current_anim.frameNum = 0;
	current_anim.composeFunc = composeFunc;
	current_anim.timeContinue = SDL_GetTicks() + 100;

	// Initialize canvas for this animation
	grs_bitmap bm;
	uchar *bptr = (uchar *)malloc(head->size.x * head->size.y * 2);

	gr_init_bm(&bm, bptr, BMT_FLAT8, BMF_TRANS, head->size.x, head->size.y);
    gr_make_canvas(&bm, &current_anim.cnv);
    gr_push_canvas(&current_anim.cnv);
    gr_clear(0);
    gr_pop_canvas();

	return &current_anim;
}

void AnimSetNotify(ActAnim *paa, void *powner, AnimCode mask,
        void (*func) (ActAnim *, AnimCode ancode, AnimCodeData *animData)) {

	// user callback function
	paa->notifyFunc = func;
}

void AnimKill(ActAnim *paa) {
	// Stop animation
    AnimCodeData data;

    if(current_anim.notifyFunc != NULL)
		current_anim.notifyFunc(&current_anim, ANCODE_KILL, &data);

	free(current_anim.cnv.bm.bits);
}
