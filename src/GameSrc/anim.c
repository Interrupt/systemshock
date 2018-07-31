#include "anim.h"
#include "tools.h"
#include "faketime.h"

ActAnim current_anim;

long next_time = 0;

// play
void AnimRecur() {
	// advance to next frame
	DEBUG("Playing frame %x!", current_anim.currFrameRef);

	draw_full_res_bm(current_anim.currFrameRef, 0, 0, FALSE);

	next_time++;
	if(next_time > 10) {
		current_anim.currFrameRef++;
		next_time = 0;
	}

	extern void SDLDraw(void);
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
	// user callback function
}

void AnimKill(ActAnim *paa) {
	// Stop animation
}
