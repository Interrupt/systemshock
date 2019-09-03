/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.
Copyright (C) 2019 Shockolate Project

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/*
 * $Source: n:/project/lib/src/input/RCS/mouse.c $
 * $Revision: 1.15 $
 * $Author: mahk $
 * $Date: 1994/06/21 06:16:42 $
 *
 * Mouse handler adapted from Rex Bradford's Freefall mouse code.
 *
 * This file is part of the input library.
 */

//    This file only vaguely resembles the freefall mouse code from
//    which it descended.  It supports some very-low-level input routines
//    for the mouse.  It provides an interrupt-driven event queue, polling,
//    and callbacks from the interrupt handler.

//	---------------------------------------------------------
// 6/21/94 ML Added mouse velocity support in mousevel.h so that we can emulate the mouse
// using other devices.
//	---------------------------------------------------------
// For the Mac version I use a TimeManager task to poll the mouse for mouse
// movement callback routines.  Mouse click events will be handled throught the normal
// Macintosh event queue.  Most of the stuff in this file will go away.
// ¥¥¥Note:  The mouse position will always be returned in *local* coordinates,
// that is, local to the main game window.

#include <SDL.h>

#include "error.h"
#include "mouse.h"
#include "tickcount.h"

typedef struct _mouse_state {
    short x, y;
    short butts;
} mouse_state;

/*
#define DEFAULT_XRATE 16  			// Default mouse sensitivity parameters
#define DEFAULT_YRATE 8
#define DEFAULT_ACCEL 100
#define LO_RES_SCREEN_WIDTH 320
#define HIRES_XRATE 2
#define HIRES_YRATE 2

//	These are global for fast access from interrupt routine & others

short gMouseCritical;						// in critical region?
*/
#define NUM_MOUSEEVENTS 32
short mouseQueueSize = NUM_MOUSEEVENTS;
volatile short mouseQueueIn;                // back of event queue
volatile short mouseQueueOut;               // front of event queue
ss_mouse_event mouseQueue[NUM_MOUSEEVENTS]; // array of events

short mouseInstantX; // instantaneous mouse xpos (int-based)
short mouseInstantY; // instantaneous mouse ypos (int-based)
short mouseInstantButts;
/*
short mouseButtMask;   					// amt to mask to get buttons.
ubyte mouseXshift = 0;  					// Extra bits of mouse resolution
ubyte mouseYshift = 1;
*/
ubyte mouseMask = 0xFF; // mask of events to put in the queue.
/*
uchar  mouseLefty = FALSE; 				// is the user left-handed?
*/
#define NUM_MOUSE_CALLBACKS 16
mouse_callfunc mouseCall[NUM_MOUSE_CALLBACKS];
void *mouseCallData[NUM_MOUSE_CALLBACKS];
short mouseCalls = 0; // current number of mouse calls.
short mouseCallSize = sizeof(mouse_callfunc);
/*
uchar mouse_installed = FALSE;					// was mouse found?

ulong default_mouse_ticks = 0;
ulong volatile *mouse_ticks = &default_mouse_ticks;  // Place to get mouse timestamps.

// MOUSE VELOCITY STUFF

int mouseVelX = 0, mouseVelY = 0;
int mouseVelXmax = 0x7FFFFFFF;
int mouseVelYmax = 0x7FFFFFFF;
int mouseVelXmin = 0x80000000;
int mouseVelYmin = 0x80000000;

//	Macros & defines

#define MOUSECRITON() (gMouseCritical++)
#define MOUSECRITOFF() (gMouseCritical--)

#define INT_MOUSE 0x33		// mouse software interrupt vector

#define ERRET 1



extern void MouseHandler(void);
extern ulong mouseHandlerSize;
*/

// extern short gActiveLeft, gActiveTop;
// bool gRBtnWasDown = true;
extern uchar pKbdGetKeys[16];

//----------------
// Internal Prototypes
//----------------
static void ReadMouseState(mouse_state *pMouseState);

#if __profile__
#pragma profile off
#endif
//---------------------------------------------------------------
//  The following section is the time manager task for handling mouse movement.
//---------------------------------------------------------------
#pragma require_prototypes off

// KLC - try calling this from the main timer task.
//---------------------------------------------------------------
void MousePollProc(void) {
    // TODO: is this even still needed? if so, can it be replaced by setting mouseInstant* in pump_events() ?
    //       if the callbacks from mouseCall[] are still needed, could they also be called in pump_events() ?
    //       if not, could they be the only thing called here, while mouseInstant* is still set in pump_events() ?

    extern ss_mouse_event latestMouseEvent;
    mouseInstantButts = latestMouseEvent.buttons;

    if (mouseInstantX != latestMouseEvent.x || mouseInstantY != latestMouseEvent.y) // If different
    {
        mouseInstantX = latestMouseEvent.x; // save the position
        mouseInstantY = latestMouseEvent.y;

        ss_mouse_event e = latestMouseEvent;
        e.type = MOUSE_MOTION;

        for (uint16_t i = 0; i < mouseCalls; i++)
            if (mouseCall[i] != NULL)
                mouseCall[i](&e, mouseCallData[i]);

        if (mouseMask & MOUSE_MOTION) // Add a mouse-moved event
        {                             // to the internal queue.
            short newin = mouseQueueIn, newout = mouseQueueOut;
            short in = newin;
            mouseQueue[newin] = e;
            newin = (newin + 1 < mouseQueueSize) ? newin + 1 : 0;
            if (newin == mouseQueueOut)
                newout = (newout + 1 < mouseQueueSize) ? newout + 1 : 0;
            mouseQueueOut = newout;
            mouseQueueIn = newin;
        }
    }

    /*Point					mp;
    short					i;
    mouse_event		e;

    mp = *(Point *)0x830;												// Get mouse location from low
memory. mp.h -= gActiveLeft;
// Convert to "local" screen coordinates. mp.v -= gActiveTop;

    GetKeys((UInt32 *)pKbdGetKeys);								// Check keys to see if our
simulated
                                                                                                                                                                    // right button is down.
    if (Button())																// See if the mouse button is
down
    {
            mouseInstantButts = 1;
            if ((pKbdGetKeys[0x3A>>3] >> (0x3A & 7)) & 1)		// If the option key is down also,
                    mouseInstantButts = 2;										// then it's really a right-button
click.
    }
    if( ((pKbdGetKeys[0x31>>3] >> (0x31 & 7)) & 1) ||		// If space, enter, or return are down,
             ((pKbdGetKeys[0x4C>>3] >> (0x4C & 7)) & 1) ||		// then pretend right-button is down.
             ((pKbdGetKeys[0x24>>3] >> (0x24 & 7)) & 1)  )
            mouseInstantButts |= 2;

    if (mouseInstantX != mp.h || mouseInstantY != mp.v)		// If different
    {
            mouseInstantX = mp.h;												// save the
position mouseInstantY = mp.v;

            e.x = mp.h;																// and inform the callback
routines e.y = mp.v; e.type = MOUSE_MOTION; e.buttons = mouseInstantButts; for (i = 0; i < mouseCalls; i++)
                    if(mouseCall[i] !=NULL)
                            mouseCall[i](&e,mouseCallData[i]);

            if (mouseMask & MOUSE_MOTION)							// Add a mouse-moved
event
            {																			// to the internal
queue. short newin = mouseQueueIn, newout = mouseQueueOut; short in = newin; mouseQueue[newin] = e; newin =  (newin + 1
< mouseQueueSize) ? newin + 1 : 0; if (newin == mouseQueueOut) newout = (newout + 1 < mouseQueueSize) ? newout + 1 : 0;
                    mouseQueueOut = newout;
                    mouseQueueIn  = newin;
            }
    }
//	PrimeTime((QElemPtr)tmTaskPtr, 50);						// Check 20 times a second.

*/
}

#pragma require_prototypes on
#if __profile__
#pragma profile on
#endif

//	---------------------------------------------------------
//	mouse_shutdown() terminates mouse handler.
//	---------------------------------------------------------
// For Mac version: do nothing.

errtype mouse_shutdown(void) {
    /*	union REGS regs;
            struct SREGS segregs;

            Spew(DSRC_MOUSE_Shutdown,("entering mouse_shutdown()\n"));


            //	Shut down Microsoft mouse driver

            if (mouse_installed)
            {
                    regs.x.eax = 0x000C;
                    regs.x.ecx = 0;
                    regs.x.edx = 0;
                    segregs.es = 0;
                    segregs.ds = 0;
                    int386x(INT_MOUSE, &regs, &regs, &segregs);

                    dpmi_unlock_lin_region(MouseHandler,mouseHandlerSize);
            } */
    //	RmvTime((QElemPtr)&pMousePollTask);							// Stop the mouse polling
    //task 	DisposeRoutineDescriptor(pMousePollPtr);						// Dispose its UPP

    return OK;
}

//	---------------------------------------------------------
//	mouse_init() initializes mouse handler.  It does the following:
//
//	1. Microsoft mouse driver initialized
//	2. Customizes mouse driver & handler based on display mode
//	3. Initializes mouse handler state variables
//	---------------------------------------------------------
//  For Mac version: ignore sizes (mouse is already set up).

errtype mouse_init(short mone, short mtwo) {
    mouse_state mstate;
    /*
            union REGS regs;
            struct SREGS segregs;
            Spew(DSRC_MOUSE_Init,("Entering mouse_init()\n"));

            //	Initialize Microsoft mouse driver

            regs.x.eax = 0x0000;
            int386(INT_MOUSE, &regs, &regs);
            mouse_installed = (regs.w.ax != 0);

            //	If mouse found, do more initialization

            if (mouse_installed)
            {

            DBG(DSRC_MOUSE_Init,
            { if (!mouse_installed)
                    Warning(("mouse_init(): Mouse not installed\n"));
            })
    */
    //	Initialize mouse state variables

    extern void sdl_mouse_init(void);
    sdl_mouse_init();

    mouseQueueIn = 0;
    mouseQueueOut = 0;

    mouseCalls = 0;

    ReadMouseState(&mstate);
    mouseInstantX = mstate.x;
    mouseInstantY = mstate.y;
    mouseInstantButts = mstate.butts;
    /*
            //	Set up mouse interrupt handler

            dpmi_lock_lin_region(MouseHandler,mouseHandlerSize);

            regs.x.eax = 0x000C;
            regs.w.cx = 0xFF;							// take all mouse events
            regs.x.edx = FP_OFF(MouseHandler);
            segregs.es = FP_SEG(MouseHandler);
            segregs.ds = segregs.es;
            int386x(INT_MOUSE, &regs, &regs, &segregs);

            // Do the sensitivity scaling thang.
            mouse_set_screensize(xsize,ysize);
            }

            AtExit(mouse_shutdown);

            //	Return whether or not mouse found

            return(mouse_installed ? OK : ERR_NODEV);
    */

    /*
            pMousePollPtr = NewTimerProc(MousePollProc);			// Make a UPP for the TM task

            pMousePollTask.task.tmAddr = pMousePollPtr;				// Insert the mouse polling TM task
            pMousePollTask.task.tmWakeUp = 0;
            pMousePollTask.task.tmReserved = 0;
    #ifndef __powerc
            pMousePollTask.appA5 = SetCurrentA5();
    #endif
            InsTime((QElemPtr)&pMousePollTask);
            PrimeTime((QElemPtr)&pMousePollTask, 50);				// Check 20 times a second
    */
    return (OK);
}
/*
// ---------------------------------------------------------
// mouse_set_screensize() sets the screen size, scaling mouse sensitivity.
errtype mouse_set_screensize(short x, short y)
{
   short xrate = DEFAULT_XRATE,yrate = DEFAULT_YRATE,t = DEFAULT_ACCEL;
   if (x > LO_RES_SCREEN_WIDTH)
   {
      xrate = HIRES_XRATE;
      yrate = HIRES_YRATE;
      mouseXshift = 3;
      mouseYshift = 3;
   }
   else
   {
      xrate /= 2;
      mouseXshift = 1;
      mouseYshift = 0;
   }
   mouse_set_rate(xrate,yrate,t);
   mouse_constrain_xy(0,0,x-1,y-1);
   mouse_put_xy(mouseInstantX,mouseInstantY);
   return OK;
}
*/
/*
//---------------------------------------------------------
// _mouse_update_vel() updates coordinates based on mouse
//  velocity.  Generates a motion event if there's any change to position.

void _mouse_update_vel(void)
{
   static ulong last_ticks = 0;

   ulong ticks = *mouse_ticks;

   if (ticks != last_ticks && (mouseVelX != 0 || mouseVelY != 0))
   {
      short newx = mouseInstantX;
      short newy = mouseInstantY;
      ulong dt = ticks - last_ticks;
      short dx = (mouseVelX*dt) >> MOUSE_VEL_UNIT_SHF;
      short dy = (mouseVelY*dt) >> MOUSE_VEL_UNIT_SHF;

      mouse_put_xy(newx+dx,newy+dy);
   }
   last_ticks = ticks;
}
*/

// --------------------------------------------------------
// mouse_check_btn checks button state.
//   res = ptr to result
//   button = button number 0-2
//	---------------------------------------------------------
//  For Mac version: Basically just return true or false right now.
// WH: no use
#if 0
errtype mouse_check_btn(short button, bool *res) {

    if (button == 1) {
        *res = SDL_BUTTON(SDL_BUTTON_LEFT);
    } else if (button == 2) {
        *res = SDL_BUTTON(SDL_BUTTON_RIGHT);
    }
    /*   if (!mouse_installed)
       {
          Warning(("mouse_get_xy(): mouse not installed.\n"));
          return ERR_NODEV;
       }
       *res = (mouseInstantButts >> button) & 1;
       Spew(DSRC_MOUSE_CheckBtn,("mouse_check_btn(%d,%x) *res = %d\n",button,res,*res)); */
    return OK;
}
#endif
// ---------------------------------------------------------
// mouse_look_next gets the event in front the event queue,
// but does not remove the event from the queue.
// res = ptr to event to be filled.
//	---------------------------------------------------------
//  For Mac version: Check the normal Mac event queue for mouse events.  The events
//  looked for depend on the 'mouseMask' setting.
// WH: no use
#if 0
errtype mouse_look_next(ss_mouse_event *res) {
    printf("mouse_look_next not implemented.\n");

    /*if (OSEventAvail(eventMask, &theEvent))				// If there is an event,
    {
            GlobalToLocal(&theEvent.where);
            res->x = theEvent.where.h;								// fill in the mouse_event
    record. res->y = theEvent.where.v; res->timestamp = theEvent.when;
            if (theEvent.modifiers & optionKey)					// If the option keys is down, send back
    a
            {																	// right-button
    event. if (theEvent.what == mouseDown) res->type = MOUSE_RDOWN; else if (theEvent.what == mouseUp) res->type =
    MOUSE_RUP; res->buttons = 2; res->modifiers = 0;
            }
            else																// Otherwise it's a left-button
    event.
            {
                    if (theEvent.what == mouseDown)
                            res->type = MOUSE_LDOWN;
                    else if (theEvent.what == mouseUp)
                            res->type = MOUSE_LUP;
                    res->buttons = 1;
                    res->modifiers = (uchar)(theEvent.modifiers >> 8);
            }
    }*/

    // If there's not a mouse click event, check the internal queue for mouse
    // movement events.
    /*else if (mouseMask & MOUSE_MOTION)
    {
            if (mouseQueueOut == mouseQueueIn)			// If no motion events, return an error.
                    return ERR_NODEV;
            else
                    *res = mouseQueue[mouseQueueOut];		// Return the event.
    }

    // If there are no events at all, return an error.
    else
            return ERR_NODEV;*/

    /*
       Spew(DSRC_MOUSE_LookNext,("entering mouse_look_next()\n"));
       if (mouseQueueOut == mouseQueueIn)
          _mouse_update_vel();
       if (mouseQueueOut == mouseQueueIn)
       {
          Spew(DSRC_MOUSE_LookNext,("mouse_look_next(): Queue Underflow.\n"));
          return ERR_NODEV;
       }
      *res = mouseQueue[mouseQueueOut];
    */
    return OK;
}
#endif

/*
// -------------------------------------------------------
//
// mouse_generate() adds an event to the back of the
//  mouse event queue.  If this overflows the queue,

errtype mouse_generate(mouse_event e)
{
   short newin = mouseQueueIn, newout = mouseQueueOut;
   short in = newin;
   int i;
   errtype result = OK;
   Spew(DSRC_MOUSE_Generate,("Entering mouse_generate()\n"));
   mouseQueue[newin] = e;
   newin =  (newin + 1  < mouseQueueSize) ? newin + 1 : 0;
   if (newin == mouseQueueOut)
   {
      newout = (newout + 1 < mouseQueueSize) ? newout + 1 : 0;
      Spew(DSRC_MOUSE_Generate,("mouse_generate(): Queue Overflow.\n"));
      result = ERR_DUNDERFLOW;
   }

   mouseQueueOut = newout;
   mouseQueueIn  = newin;
   mouseInstantX = e.x;
   mouseInstantY = e.y;
   mouseInstantButts = e.buttons;
   for (i = 0; i < mouseCalls; i++)
      if(mouseCall[i] !=NULL)
         mouseCall[i](&mouseQueue[in],mouseCallData[i]);
   return result;
}
*/

// ------------------------------------------------------
//
// mouse_set_callback() registers a callback with the interrupt handler
// f = func to be called back.
// data = data to be given to the func when called
// *id = set to a unique id of the callback.

errtype mouse_set_callback(mouse_callfunc f, void *data, int *id) {
    //   Spew(DSRC_MOUSE_SetCallback,("entering mouse_set_callback(%x,%x,%x)\n",f,data,id));
    for (*id = 0; *id < mouseCalls; ++*id)
        if (mouseCall[*id] == NULL)
            break;
    if (*id == NUM_MOUSE_CALLBACKS) {
        //		Spew(DSRC_MOUSE_SetCallback,("mouse_set_callback(): Table Overflow.\n"));
        return ERR_DOVERFLOW;
    }
    if (*id == mouseCalls)
        mouseCalls++;
    //	Spew(DSRC_MOUSE_SetCallback,("mouse_set_callback(): *id = %d, mouseCalls = %d\n",*id,mouseCalls));
    mouseCall[*id] = f;
    mouseCallData[*id] = data;
    return OK;
}

// -------------------------------------------------------
//
// mouse_unset_callback() un-registers a callback function
// id = unique id of function to unset

errtype mouse_unset_callback(int id) {
    // Spew(DSRC_MOUSE_UnsetCallback,("entering mouse_unset_callback(%d)\n",id));
    if (id >= mouseCalls || id < 0) {
        // Spew(DSRC_MOUSE_UnsetCallback,("mouse_unset_callback(): id out of range \n"));
        return ERR_RANGE;
    }
    mouseCall[id] = NULL;
    while (mouseCalls > 0 && mouseCall[mouseCalls - 1] == NULL)
        mouseCalls--;
    return OK;
}

// --------------------------------------------------------
//
// mouse_constrain_xy() defines min/max coords
//  ¥¥¥ don't do anything for now.  Will need to implement some day.
errtype mouse_constrain_xy(short xl, short yl, short xh, short yh) {
    /*
       union REGS regs;
       Spew(DSRC_MOUSE_ConstrainXY,("mouse_constrain_xy(%d,%d,%d,%d)\n",xl,yl,xh,yh));
       if (!mouse_installed)
       {
          Warning(("mouse_constrain_xy(): mouse not installed.\n"));
          return ERR_NODEV;
       }
       regs.x.eax = 0x0007;
       regs.x.ecx = xl << mouseXshift;
       regs.x.edx = xh << mouseXshift;
       int386(INT_MOUSE,&regs,&regs);
       regs.x.eax = 0x0008;
       regs.x.ecx = yl << mouseYshift;
       regs.x.edx = yh << mouseYshift;
       int386(INT_MOUSE,&regs,&regs);
    */
    return OK;
}

/*
// --------------------------------------------------------
//
// mouse_set_rate() sets mouse rate, doubling threshhold

errtype mouse_set_rate(short xr, short yr, short thold)
{
   union REGS regs;
   Spew(DSRC_MOUSE_SetRate,("mouse_set_rate(%d,%d,%d)\n",xr,yr,thold));
   if (!mouse_installed)
   {
      Warning(("mouse_set_rate(): mouse not installed.\n"));
      return ERR_NODEV;
   }
//   if (mouseXshift > 0) xr = xr / mouseXshift;  // why are we dividing?  Because shifting is too extreme
//   if (mouseYshift > 0) yr = yr / mouseYshift;
   regs.x.eax = 0x000F;
   regs.x.ecx = max(1,xr);
   regs.x.edx = max(1,yr);
   int386(INT_MOUSE,&regs,&regs);
   regs.x.eax = 0x0013;
   regs.x.edx = max(1,thold);
   int386(INT_MOUSE,&regs,&regs);
   return OK;
}

// --------------------------------------------------
//
// mouse_get_rate() gets current sensitivity values

errtype mouse_get_rate(short* xr, short* yr, short* thold)
{
   union REGS regs;
   regs.x.eax = 0x001B;
   int386(INT_MOUSE,&regs,&regs);
   *xr = regs.x.ebx;
   *yr = regs.x.ecx;
   *thold = regs.x.edx;
//   if (mouseXshift > 0) *xr *= mouseXshift;  // why are we multiplying?  Because shifting is too extreme
//   if (mouseYshift > 0) *yr *= mouseYshift;
   return OK;
}


// --------------------------------------------------------
//
// mouse_set_timestamp_register() tells the mouse library where to get
// timestamps.

errtype mouse_set_timestamp_register(ulong* tstamp)
{
   mouse_ticks = tstamp;
   return OK;
}
*/

// --------------------------------------------------------
// mouse_get_time() returns the current mouse timestamp
//	--------------------------------------------------------
// For Mac version:  Just return TickCount().

uint32_t mouse_get_time(void) {
    return TickCount();
}

//	--------------------------------------------------------
//	ReadMouseState() reads current state of mouse.
//
//		pMouseState = ptr to mouse state struct, filled in by routine
//	--------------------------------------------------------
// For Mac version:  Use Mac routines to get mouse position and state.

static void ReadMouseState(mouse_state *pMouseState) {
    int mouse_x;
    int mouse_y;

    uint mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
    pMouseState->x = mouse_x;
    pMouseState->y = mouse_y;
    pMouseState->butts = 0;

    if (mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
        pMouseState->butts = 1;
    }

    if (mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
        pMouseState->butts = 2;
    }

    /*	union REGS regs;

            regs.x.eax = 0x0003;
            int386(INT_MOUSE, &regs, &regs);

            pMouseState->x = regs.w.cx;
            pMouseState->y = regs.w.dx;
            pMouseState->butts = regs.w.bx; */
}

/*
// ---------------------------------------------------
//
// mouse_extremes() finds the min and max "virtual" coordinates of the mouse position

errtype mouse_extremes( short *xmin, short *ymin, short *xmax, short *ymax )
{
   union REGS regs;

   regs.x.eax = 0x31;
   int386( INT_MOUSE, &regs, &regs );

   *xmin = regs.w.ax >> mouseXshift;
   *ymin = regs.w.bx >> mouseYshift;
   *xmax = regs.w.cx >> mouseXshift;
   *ymax = regs.w.dx >> mouseYshift;

   Spew( DSRC_MOUSE_Extremes, ("mouse_extremes(): <%d %d> to <%d %d>\n", *xmin, *ymin, *xmax, *ymax ));

   return OK;
}


// ------------------------------------------------------
//
// mouse_set_lefty() sets mouse handedness

#define SHIFTDIFF 1

static short shifted_button_state(short bstate)
{
   short tmp = (bstate & MOUSE_RBUTTON) >> SHIFTDIFF;
   tmp |= (bstate & MOUSE_LBUTTON) << SHIFTDIFF;
   tmp |= bstate & MOUSE_CBUTTON;
   return tmp;
}

errtype mouse_set_lefty(uchar lefty)
{
   if (lefty == mouseLefty) return ERR_NOEFFECT;
   mouseInstantButts = shifted_button_state(mouseInstantButts);
   mouseLefty = lefty;
   return OK;
}




// ---------------------------------------------------
//
// mouse_set_velocity_range() sets the range of valid mouse pointer velocities.

errtype mouse_set_velocity_range(int xl, int yl, int xh, int yh)
{
   mouseVelXmin = xl;
   mouseVelYmin = yl;
   mouseVelXmax = xh;
   mouseVelYmax = yh;
   return OK;
}

errtype mouse_set_velocity(int x, int y)
{
   mouseVelX = max(mouseVelXmin,min(x,mouseVelXmax));
   mouseVelY = max(mouseVelYmin,min(y,mouseVelYmax));
   if (mouseVelX != x || mouseVelY != y)
      return ERR_RANGE;
   return OK;
}

errtype mouse_add_velocity(int x, int y)
{
   return mouse_set_velocity(mouseVelX + x, mouseVelY + y);
}

errtype mouse_get_velocity(int* x, int* y)
{
   *x = mouseVelX;
   *y = mouseVelY;
   return OK;
}
*/
