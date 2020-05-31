#ifndef ANIM_H
#define ANIM_H

#include <rect.h>
//#include <llist.h>
#include <res.h>
#include <2d.h>
#include <2dres.h>
#include <fix.h>

//      Animation event codes

typedef uchar AnimCode;

#define ANCODE_NEWFRAME         0x01            // new frame, update screen
#define ANCODE_END              0x04            // end anim w/o kill (no arg)
#define ANCODE_KILL             0x08            // kill anim (no arg)

typedef struct {
   uchar unknown;
   uchar frameRunStart;
   uchar frameRunEnd;
   fix frameDelay;
} AnimCodeData;

typedef struct {
        LGPoint size;                           // size of anim
        Id frameSetId;                          // resource id of binary frame set
        uchar unknown1[6];
        short unknown2;
        AnimCodeData data[];
} AnimHead;

//      ActAnim: describes an active animation record

typedef struct ActAnim_ {
        LGRegion                        *reg;
        AnimHead                        *pah;           // ptr to animation header
        LGPoint                         loc;
        grs_canvas                      cnv;
        void                            (*notifyFunc) (struct ActAnim_ *paa, AnimCode ancode, AnimCodeData *animData); // owner evt handler
        void                            (*composeFunc) (LGRect *area, ubyte flags); // owner compose handler
        Ref                             currFrameRef;   // current frame ref
        int                             curSeq;
        int                             frameNum;
        fix                             animRate;       // animation rate
        ulong                           timeContinue;   // time at which to advance to next step
        void                            *dataBuffer;    // data buffer to use for frames, or NULL
        long                            dataBufferLen;  // length of data buffer
} ActAnim;

//      General prototypes: anim.c

void AnimRecur();                                                       // update animations in progress

//      Play and control anims: anim.c

ActAnim *AnimPlayRegion(Ref animRef, LGRegion *region, LGPoint loc, char unknown, // play anim into canvas
   void (*composeFunc)(LGRect *area, ubyte flags));

void AnimKill(ActAnim *paa);                                            // kill one or all anims
void AnimSetNotify(ActAnim *paa, void *powner, AnimCode mask,
        void (*func) (ActAnim *, AnimCode ancode, AnimCodeData *animData));

//      Deal with animation resources: anim.c

bool AnimPreloadFrames(ActAnim *paa, Ref animRef);                      // preload an anim's frames

//      Macro to read anim header from resource

#define AnimReadHeader(ref,pAnhead) (*pAnhead = (* ((AnimHead *) RefGet(ref))));

//      Convenience macros

#define AnimSetDataBuffer(paa, buffer) { \
        (paa)->dataBuffer = (buffer); (paa)->dataBufferLen = 0x7FFFFFFFL; }
#define AnimSetDataBufferSafe(paa, buffer, len) { \
        (paa)->dataBuffer = (buffer); (paa)->dataBufferLen = (len); }

#endif
