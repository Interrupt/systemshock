/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.

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
#ifndef _EVENT_H
#define _EVENT_H 
#include "lg.h"
#include "error.h"
#include "slab.h"
#include "region.h" 
#include "mouse.h"

// ---------------
// INPUT EVENTS 
// ---------------

// Type-specific data for a raw key event.
typedef struct
{
  short  scancode; // subtype
  uchar  action;   // KBS_UP or _DOWN
  ushort mods;     // KLC - modifiers added for Mac version
} uiRawKeyData;

// Poll key event is the same structure.
typedef uiRawKeyData uiPollKeyData;


// Cooked key events 
typedef struct
{
  short code;        /* cooked keycode, chock full o' stuff */
} uiCookedKeyData;

// mouse events
typedef struct
{
  short    action;   /* mouse event type, as per mouse library */
  uint32_t tstamp;
  ubyte    buttons; 
  uchar    modifiers; 
} uiMouseData;

// joystick events
typedef struct
{
  short   action;     /* joystick event subtype, as defined below */
  uchar   joynum;     /* joystick number */
  LGPoint joypos;     /* joystick position */
} uiJoyData;

// User-defined event can hold a pointer or integer.
typedef struct
{
    short    subtype;
    intptr_t data;
} uiUserData;

// Generalized input event struct
typedef struct _ui_event
{
    LGPoint  pos;
    uint32_t type;
    union
    {
	short subtype;
	uiRawKeyData    raw_key_data;
	uiPollKeyData   poll_key_data;
	uiCookedKeyData cooked_key_data;
	uiMouseData     mouse_data;
	uiJoyData       joystick_data;
	uiUserData      user_data;
    };
} uiEvent;


// Type field values 
#define UI_EVENT_NULL           0x00000000
#define UI_EVENT_KBD_RAW        0x00000001
#define UI_EVENT_KBD_COOKED     0x00000002
#define UI_EVENT_KBD_POLL       0x00000020
#define UI_EVENT_MOUSE          0x00000004
#define UI_EVENT_MOUSE_MOVE     0x00000008
#define UI_EVENT_JOY            0x00000010
#define UI_EVENT_MIDI           0x10000000  // Hey, gotta be ready for the future.
#define UI_EVENT_USER_DEFINED   0x80000000
#define ALL_EVENTS              0xFFFFFFFF

// extended mouse event types (double clicks)
#define UI_MOUSE_LDOUBLE   (1 << 7)
#define UI_MOUSE_RDOUBLE   (1 << 8)
#define UI_MOUSE_CDOUBLE   (1 << 9)
#define UI_MOUSE_BTN2DOUBLE(i) (128 << (i))

// The "first" events are generated for the first half of a 
// double click, so you can start reacting to the first click 
// without waiting for the second. 

#define UI_MOUSE_FIRST_LDOWN  (1 << 10)
#define UI_MOUSE_FIRST_RDOWN  (1 << 11)
#define UI_MOUSE_FIRST_CDOWN  (1 << 12)
#define UI_MOUSE_BTN2FIRST_DOWN(i) (1 << ((i)+10))

#define UI_MOUSE_FIRST_LUP    (1 << 13)
#define UI_MOUSE_FIRST_RUP    (1 << 14)
#define UI_MOUSE_FIRST_CUP    (1 << 15)
#define UI_MOUSE_BTN2FIRST_UP(i) (1 << ((i)+13))


#define UI_JOY_MOTION         0
#define UI_JOY_BUTTON1UP      1
#define UI_JOY_BUTTON2UP      2
#define UI_JOY_BUTTON1DOWN    3 
#define UI_JOY_BUTTON2DOWN    4

// ----------------
//  EVENT HANDLERS
// ----------------

/* Event handlers are installed by the client to receive callbacks when events
   happen.  Event handlers are called when the ui toolkit is 
   polled.  Interrupt-driven phenomena such as mouse cursors will, in
   general, be internal to the ui-toolkit.  An event handler is installed
   on a region, and will receive events when the mouse is in that region.
   It is possible to chain event handlers within a region.  In
   this case, an event is "offered" to each event handler, in order,
   one at a time until an event handler chooses to accept it.   */


typedef uchar (*uiHandlerProc)(uiEvent* e, LGRegion* r, intptr_t state);

// If an event-handler's proc returns true, it has accepted the event, and no
// other event handler will see the event.  An event handler will only be
// offered those events specified by its typemask; it automatically rejects
// any other events. 

errtype uiInstallRegionHandler(LGRegion* v, uint32_t evmask, uiHandlerProc proc, intptr_t state, int* id);
// installs an event handler at the front of r's handler chain.  The event handler
// will call "proc" with the event, the region "v", and the value of "state" 
// whenever "v" receives any event whose type bit is set in evmask.  
// sets *id to an id for that event handler.


errtype uiRemoveRegionHandler(LGRegion* v, int id);
// Removes the event handler with the specified id from a region's handler chain

errtype uiSetRegionHandlerMask(LGRegion* r, int id, int evmask);
// Changes the event mask for handler #id in region r.  

errtype uiShutdownRegionHandlers(LGRegion* r);
// Shut down and destroy all handlers for a region.


// --------------
// REGION OPACITY
// --------------

// The opacity of a region is the mask of event types that cannot pass through the 
// region.  Set bits in the opacity mask indicate that events of that type will be 
// automatically rejected if they reach that region, and will not be offered to any other region.  

// Se uiDefaultRegionOpacity  in the globals section

errtype uiSetRegionOpacity(LGRegion* r, ulong opacity);
// Sets the opacity of a region. 

ulong uiGetRegionOpacity(LGRegion* r);
// Gets the opacity mask of the region. 


// -----------
// INPUT FOCUS
// -----------

errtype uiGrabFocus(LGRegion* r, ulong evmask);
// grabs input focus on the active slab for region r for events specified by evmask

errtype uiReleaseFocus(LGRegion* r, ulong evmask);
// If r has the current input focus in the active slab, then releases r's 
// focus on the events specified by  evmask, and restores the previous focus.  Else does nothing.

errtype uiGrabSlabFocus(uiSlab* slab, LGRegion* r, ulong evmask);
// Grabs focus for region r on the specified slab. 

errtype uiReleaseSlabFocus(uiSlab* slab, LGRegion* r, ulong evmask);
// If r has the current input focus in the specified slab, then releases r's 
// focus on the events specified by  evmask, and restores the previous focus.  
// Else does nothing.


// -----------------------
// POLLING AND DISPATCHING
// -----------------------

errtype uiPoll(void);
// polls the ui toolkit, dispatching all events.  


errtype uiQueueEvent(uiEvent* ev);
// adds an event to the ui event queue.  The event will be dispatched at the next uiPoll() call

uchar uiDispatchEvent(uiEvent* ev);
// Dispatches an event right away, without queueing. Returns 
// Whether or not the event was accepted by a handler. 

uchar uiDispatchEventToRegion(uiEvent* ev, LGRegion* r);
// Like uiDispatchEvent, but dispatches an event to a 
// specific region's event handlers.   

errtype uiSetMouseMotionPolling(uchar poll);
// Iff poll is true, exactly one mouse motion event will be  generated 
// per call to uiPoll, the motion event will be generated by polling the 
// mouse position.  Otherwise, all motion events generated by the interrupt handler will
// be dispatched, and thus no motion event will be dispatched if the mouse has not moved. 
// Defaults to FALSE

errtype uiMakeMotionEvent(uiEvent* ev);
// Fills *ev with a mouse motion event reflecting the current mouse position.

errtype uiSetKeyboardPolling(uchar* codes);
// Codes is a KBC_NONE terminated array of scancodes to be polled by the system.
// if  a code is in the list, the specified key will generate one keyboard polling event
// (type UI_EVENT_KBD_POLL) per call to uiPoll.  otherwise, no such event will be generated 
// for this key.  
// defaults to NULL. 

errtype uiFlush(void);
// Flushes all ui system input events.

uchar uiCheckInput(void);
// reads through the input queue, returning true if there
// is a key or mouse button up event, false otherwise.

// ---------------------------
// INITIALIZATION AND SHUTDOWN
// ---------------------------

errtype uiInit(uiSlab* slab);
// Initialize the ui toolkit.  
// Sets the current slab.  

void uiShutdown(void);
// shuts down the ui toolkit.  



// ----------------
//     GLOBALS
// ----------------

extern ushort uiDoubleClickTime;
// The maximum time separation between individual clicks of a double click

extern ushort uiDoubleClickDelay;
// The maximum allowed time between the first down and up event in a double click

extern uchar uiDoubleClicksOn[NUM_MOUSE_BTNS];
// are double clicks allowed for the specified button?
// defaults to FALSE

extern uchar uiAltDoubleClick;
// Whether alt-click should emulate double click.
// Defaults to FALSE;

extern LGRegion* uiLastMouseRegion[NUM_MOUSE_BTNS];
// Stores a pointer to the region that accepted the 
// last down event for each button. 

extern ushort uiDoubleClickTolerance;
// How much mouse motion will we tolerate before discarding
// a potiential double click.  Defaults to 5.

extern ulong uiGlobalEventMask;
// Global mask of what events are to be dispatched.  
// initially set to ALL_EVENTS

extern ulong uiDefaultRegionOpacity;
// The initial value of the opacity of a region, used until 
// an opacity is set for the region.  
// Defaults to zero.
// When a handler is added to a region, the opacity is set to the
// value of uiDefaultRegionOpacity.  



#endif // _EVENT_H 
