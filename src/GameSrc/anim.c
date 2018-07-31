#include "anim.h"
#include "tools.h"
#include "faketime.h"
#include "gr2ss.h"

ActAnim current_anim;

long next_time = 0;

extern void SDLDraw(void);
extern void pump_events(void);

// play
void AnimRecur() {
	// advance to next frame
	// DEBUG("Playing frame %x!", current_anim.currFrameRef);

	FrameDesc *f = (FrameDesc *)RefLock(current_anim.currFrameRef);
    if (f != NULL) {
    	f->bm.bits = (uchar *)(f + 1);
    	ss_bitmap(&f->bm, 0, 0);
    }
    else {
    	DEBUG("Done playing anim!");
    	AnimCodeData data;
    	current_anim.notifyFunc(&current_anim, ANCODE_KILL, &data);
    }
    RefUnlock(current_anim.currFrameRef);

	next_time++;
	if(next_time > 5) {
		current_anim.currFrameRef++;
		next_time = 0;
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

ActAnim *AnimPlayRegion(Ref animRef, LGRegion *region, Point loc, char unknown,
   void (*composeFunc)(ActAnim *paa, AnimCode ancode, AnimCodeData *animData)) {
	// start playing

	DEBUG("Playing animation: %x", animRef);

	AnimHead *head = (AnimHead *)RefGet(animRef);
	if(head != NULL) {
		DEBUG("Animation header: %x", head->frameSetId);
	}

	current_anim.pah = head;
	current_anim.currFrameRef = MKREF(head->frameSetId, 0);

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
