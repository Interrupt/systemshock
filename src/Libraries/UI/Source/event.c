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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>

#include "lg.h"
#include "mouse.h" 
#include "kb.h"
#include "kbcook.h" 
#include "array.h" 
#include "rect.h"
#include "slab.h"
#include "event.h" 
#include "dbg.h"
//#include <_ui.h>
#include "vmouse.h"


// ---------------------
// INTERNAL PROTOTYPES
// ---------------------
void event_queue_add(uiEvent* e);
uchar event_queue_next(uiEvent** e);
uchar region_check_opacity(LGRegion* reg, ulong evmask);
uchar event_dispatch_callback(LGRegion* reg, LGRect* r, void* v);
void ui_set_last_mouse_region(LGRegion* reg,uiMouseEvent* ev);
uchar ui_try_region(LGRegion* reg, LGPoint pos, uiEvent* ev);
uchar ui_traverse_point(LGRegion* reg, LGPoint pos, uiEvent* data);
uchar send_event_to_region(LGRegion* r, uiEvent* ev);
void ui_purge_mouse_events(void);
void ui_flush_mouse_events(ulong timestamp, LGPoint pos);
void ui_dispatch_mouse_event(uiMouseEvent* mout);
void ui_poll_keyboard(void);
void ui_pop_up_keys(void);
LGPoint ui_poll_mouse_position(void);
errtype ui_init_focus_chain(uiSlab* slab);


// ---------------------
// HANDLER CHAIN DEFINES
// ---------------------

typedef struct _ui_event_handler
{
   ulong typemask;  // Which event types does this handle?    
     /* handler proc: called when a specific event is received */
   uiHandlerProc proc; 
   void *state;  // handler-specific state data 
   int next;     // used for chaining handlers.  
} uiEventHandler;

typedef struct _handler_chain
{
   Array chain;
   int front;
   ulong opacity;
} handler_chain;


#define INITIAL_CHAINSIZE  4
#define INITIAL_FOCUSES 5
#define CHAIN_END -1

ulong uiGlobalEventMask = ALL_EVENTS;

// ----------------------------
// HANDLER CHAIN IMPLEMENTATION
// ----------------------------

errtype uiInstallRegionHandler(LGRegion* r, ulong evmask, uiHandlerProc callback, void* state, int* id)
{
   handler_chain *ch;
   uiEventHandler* eh;
   int i;
   errtype err;
   // Spew(DSRC_UI_Handlers,("uiInstallRegionhandler(%x,%x,%x,%x,%x)\n",r,evmask,callback,state,id));
   if (callback == NULL || r == NULL || evmask == 0) return ERR_NULL;
   ch = (handler_chain*) r->handler;
   if (ch == NULL)
   {
      // Spew(DSRC_UI_Handlers,("uiInstallRegionHandler(): creating new handler chain\n"));
      ch = (handler_chain *)malloc(sizeof(handler_chain));
      if (ch == NULL)
      {
         // Spew(DSRC_UI_Handlers,("uiInstallRegionHandler: out of memory\n"));
         return ERR_NOMEM;
      }
      array_init(&ch->chain,sizeof(uiEventHandler),INITIAL_CHAINSIZE);
      ch->front = CHAIN_END;
      r->handler = (void*)ch;
      ch->opacity = uiDefaultRegionOpacity;
   }
   err = array_newelem(&ch->chain,&i);
   if (err != OK)
   {
      // Spew(DSRC_UI_Handlers,("uiInstallRegionHandler(): array_newelem returned %d\n",err));
      return err;
   }
   eh = &((uiEventHandler*)(ch->chain.vec))[i];
   eh->next = ch->front;
   eh->typemask = evmask;
   eh->proc = callback;
   eh->state = state;
   ch->front = i;
   ch->opacity &= ~evmask;
   *id = i;
   // Spew(DSRC_UI_Handlers,("exit uiInstallRegionHandler(): *id = %d\n",*id));
   return OK;
}

errtype uiRemoveRegionHandler(LGRegion* r, int id)
{
   errtype err;
   handler_chain* ch;
   uiEventHandler* handlers;
   int i;

   // Spew(DSRC_UI_Handlers,("uiRemoveRegionHandler(%x,%d)\n",r,id));
   if (r == NULL) return ERR_NULL;
   ch = (handler_chain*)r->handler;
   if (ch == NULL || id < 0) return ERR_RANGE;
   handlers = (uiEventHandler*)(ch->chain.vec);
   if (id == ch->front)
   {
      int next = handlers[id].next;
      err = array_dropelem(&ch->chain,id);
      if (err != OK) return err;
      ch->front = next;
      return OK;
   }
   for (i = ch->front; handlers[i].next != CHAIN_END; i = handlers[i].next)
   {
      if (handlers[i].next == id)
      {
         errtype err = array_dropelem(&ch->chain,id);
         if (err != OK) return err;
         handlers[i].next = handlers[id].next;
         return OK;
      }
   }
   return ERR_NOEFFECT;
}


errtype uiSetRegionHandlerMask(LGRegion* r, int id, int evmask)
{
   handler_chain *ch;
   uiEventHandler* handlers;
   // Spew(DSRC_UI_Handlers,("uiSetRegionHandlerMask(%x,%d,%x)\n",r,id,evmask));
   if (r == NULL) return ERR_NULL;
   ch = (handler_chain*)r->handler;
   if (ch == NULL || id >= ch->chain.fullness || id < 0) return ERR_RANGE; 
   handlers = (uiEventHandler*)(ch->chain.vec);
   handlers[id].typemask = evmask;
   return OK;
}

// -------
// OPACITY
// -------

ulong uiDefaultRegionOpacity = 0;

ulong uiGetRegionOpacity(LGRegion* reg)
{
   handler_chain *ch = (handler_chain*)(reg->handler);
   if (ch == NULL)
   {
      return uiDefaultRegionOpacity;
   }
   else
      return ch->opacity;
}

errtype uiSetRegionOpacity(LGRegion* reg,ulong mask)
{
   handler_chain *ch = (handler_chain*)(reg->handler);
   if (ch == NULL)
   {
      // Spew(DSRC_UI_Handlers,("uiSetRegionOpacity(): creating new handler chain\n"));
      ch = (handler_chain *)malloc(sizeof(handler_chain));
      if (ch == NULL)
      {
         // Spew(DSRC_UI_Handlers,("uiSetRegionOpacity: out of memory\n"));
         return ERR_NOMEM;
      }
      array_init(&ch->chain,sizeof(uiEventHandler),INITIAL_CHAINSIZE);
      ch->front = CHAIN_END;
      reg->handler = (void*)ch;
      ch->opacity = mask;
   }
   else
      ch->opacity = mask;
   return OK;
}

// -------------------
// FOCUS CHAIN DEFINES
// -------------------

typedef struct _focus_link
{
   LGRegion* reg;
   ulong evmask;
   int next;
} focus_link;

extern uiSlab* uiCurrentSlab;
#define FocusChain (uiCurrentSlab->fchain.chain)
#define CurFocus (uiCurrentSlab->fchain.curfocus)
#define FCHAIN ((focus_link*)(uiCurrentSlab->fchain.chain.vec))


// ----------------
// FOCUS CHAIN CODE
// ----------------


errtype uiGrabSlabFocus(uiSlab* slab, LGRegion* r, ulong evmask)
{
   int i;
   errtype err;
   focus_link* fchain = (focus_link*) slab->fchain.chain.vec;
   // Spew(DSRC_UI_Slab,("uiGrabSlabFocus(%x,%x,%x)\n",slab,r,evmask));
   if (r == NULL) return ERR_NULL;
   if (evmask == 0) return ERR_NOEFFECT;
   err = array_newelem(&slab->fchain.chain,&i);
   if (err != OK) return err;
   fchain[i].reg = r;
   fchain[i].evmask = evmask;
   fchain[i].next = slab->fchain.curfocus;
   // Spew(DSRC_UI_Slab,("uiGrabSlabFocus(): old focus = %d new focus = %d\n",slab->fchain.curfocus,i));
   slab->fchain.curfocus = i;
   return OK;
}


errtype uiGrabFocus(LGRegion* r, ulong evmask)
{
   return uiGrabSlabFocus(uiCurrentSlab,r,evmask);
}

errtype uiReleaseSlabFocus(uiSlab* slab, LGRegion* r, ulong evmask)
{
   errtype retval = ERR_NOEFFECT;
   focus_link* fchain = (focus_link*)slab->fchain.chain.vec;
   focus_link *l = &fchain[CurFocus];
   // Spew(DSRC_UI_Slab,("uiReleaseSlabFocus(%x,%x,%x)\n",slab,r,evmask));
   if (r == NULL) return ERR_NULL;
   if (l->reg == r)
   {
      ulong tmpmask = l->evmask & evmask;
      l->evmask &= ~evmask;
      evmask &= ~tmpmask;
      if (l->evmask == 0)
      {
         int tmp = slab->fchain.curfocus;
         slab->fchain.curfocus = l->next;
         // Spew(DSRC_UI_Slab,("uiReleaseSlabFocus(): CurFocus = %d\n",slab->fchain.curfocus));
         array_dropelem(&slab->fchain.chain,tmp);
      }
      if (evmask == 0) return OK;
      retval = OK;
       
   }
   for(; l->next != CHAIN_END; l = &fchain[l->next])
   {
      focus_link* thenext = &fchain[l->next];
      if (thenext->reg == r)
      {
         ulong tmpmask = l->evmask & evmask;
         thenext->evmask &= ~evmask;
         evmask &= ~tmpmask;
         if (thenext->evmask == 0)
         {
            int tmp = l->next;
            l->next = thenext->next;
            array_dropelem(&slab->fchain.chain,tmp);
         }
         if (evmask == 0)
            return OK;
         retval = OK;
      }
   }
   return retval;
}

errtype uiReleaseFocus(LGRegion* r, ulong evmask)
{
   return uiReleaseSlabFocus(uiCurrentSlab,r,evmask);
}


// -----------------------
// POLLING AND DISPATCHING
// -----------------------

#define INITIAL_QUEUE_SIZE 32
#define DEFAULT_DBLCLICKTIME  0
#define DEFAULT_DBLCLICKDELAY 0

ushort uiDoubleClickTime = DEFAULT_DBLCLICKTIME;
ushort uiDoubleClickDelay = DEFAULT_DBLCLICKDELAY;
uchar   uiDoubleClicksOn[NUM_MOUSE_BTNS] = { FALSE, FALSE, FALSE } ;
uchar   uiAltDoubleClick = FALSE;
ushort uiDoubleClickTolerance = 5;
static uchar   poll_mouse_motion = FALSE;
static uiMouseEvent last_down_events[NUM_MOUSE_BTNS];
static uiMouseEvent last_up_events[NUM_MOUSE_BTNS];

static struct _eventqueue
{
   int in, out;
   int size;
   uiEvent* vec;
} EventQueue;

void event_queue_add(uiEvent* e)
{
   if ((EventQueue.in + 1)%EventQueue.size == EventQueue.out)
   {
      // Queue is full, grow it.  
      int i;
      int out = EventQueue.out;
      int newsize = EventQueue.size * 2;
      uiEvent *newvec = (uiEvent *)malloc(sizeof(uiEvent)*newsize);
      for(i = 0; out != EventQueue.in; i++, out = (out+1)%EventQueue.size)
         newvec[i] = EventQueue.vec[out];
      free(EventQueue.vec);
      EventQueue.vec = newvec;
      EventQueue.size = newsize;
      EventQueue.in = i;
      EventQueue.out = 0;
   }
   EventQueue.vec[EventQueue.in] = *e;
   EventQueue.in++;
   if (EventQueue.in >= EventQueue.size) EventQueue.in = 0;
}

uchar event_queue_next(uiEvent** e)
{
   if (EventQueue.in != EventQueue.out)
   {
      *e = &EventQueue.vec[EventQueue.out++];
      if (EventQueue.out >= EventQueue.size)
         EventQueue.out = 0;
      return TRUE;
   }
   return FALSE;
}


// TRUE we are opaque to this mask.
uchar region_check_opacity(LGRegion* reg, ulong evmask)
{
   return (evmask & uiGetRegionOpacity(reg)) != 0;
}

uchar event_dispatch_callback(LGRegion* reg, LGRect* rect, void* v)
{
   uiEvent* ev = (uiEvent*)v;
   handler_chain *ch = (handler_chain*)(reg->handler);
   int i,next;

   if(ch == NULL) {
      printf("WARNING! handler_chain is NULL in event_dispatch_callback\n");
      return FALSE;
   }
   
   uiEventHandler* handlers = (uiEventHandler*)(ch->chain.vec);
   // Spew(DSRC_UI_Dispatch,("event_dispatch_callback(%x,%x,%x) event type %x\n",reg,r,v,ev->type));
/*
if (ev->type == UI_EVENT_KBD_COOKED)
{
	char buff[100];
	sprintf(buff+1, "event_dispatch_callback(%x,%x,%x) event type %x\0",reg,r,v,ev->type);
	buff[0] = strlen(buff+1);
	DebugString((uchar *)buff);
}
*/
   if (ch == NULL || handlers ==  NULL)
   {
      // Spew(DSRC_UI_Dispatch,("event_dispatch_callback(): no handler chain ch = %x handlers = %d\n",ch,handlers));
      return FALSE;
   }
   for (i = ch->front; i != CHAIN_END; i = next)
   {
      next = handlers[i].next;
      if ((handlers[i].typemask & ev->type)
            && (handlers[i].proc)(ev,reg,handlers[i].state))
      {
         // Spew(DSRC_UI_Dispatch,("Caught by handler %d\n",i));
         return TRUE;
      }
   }
   // Spew(DSRC_UI_Dispatch,("Event Rejected\n"));
   return FALSE;
}

// ui_traverse_point return values: 
#define TRAVERSE_HIT 0 
#define TRAVERSE_MISS 1
#define TRAVERSE_OPAQUE 2

LGRegion* uiLastMouseRegion[NUM_MOUSE_BTNS];


void ui_set_last_mouse_region(LGRegion* reg,uiMouseEvent* ev)
{
   int i;
   if (ev->type != UI_EVENT_MOUSE)
      return;
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      if ((ev->action & MOUSE_BTN2DOWN(i)) != 0 ||
          (ev->action & UI_MOUSE_BTN2DOUBLE(i)))
            uiLastMouseRegion[i] = reg;
      if (ev->action & MOUSE_BTN2UP(i))
         uiLastMouseRegion[i] = NULL;
   }
}

uchar ui_try_region(LGRegion* reg, LGPoint pos, uiEvent* ev)
{
   LGRect cbr;
   uchar retval = TRAVERSE_MISS;

   cbr.ul = pos;
   cbr.lr = pos;
   if (region_check_opacity(reg,ev->type)) retval = TRAVERSE_OPAQUE;
   else if (event_dispatch_callback(reg,&cbr,ev)) retval = TRAVERSE_HIT;
   else return retval;
   ui_set_last_mouse_region(reg,(uiMouseEvent*)ev);
   return retval;
}

uchar ui_traverse_point(LGRegion* reg, LGPoint pos, uiEvent* data)
{
   uchar retval = TRAVERSE_MISS;
   LGPoint rel;
   LGRegion* child;

   rel = pos;
   rel.x -= reg->abs_x;
   rel.y -= reg->abs_y;

   if ((reg->status_flags & INVISIBLE_FLAG) != 0)
      return retval;

   if (reg->event_order)
   {
      retval = ui_try_region(reg,pos,data);
      if (retval != TRAVERSE_MISS) return retval;
   }
   for (child = reg->sub_region; child != NULL; child = child->next_region)
      if (RECT_TEST_PT(child->r,rel))
      {
         retval = ui_traverse_point(child,pos,data);
         if (retval != TRAVERSE_MISS) return retval;
         break;
      }
   if (!reg->event_order)
   { 
      retval = ui_try_region(reg,pos,data);
      if (retval != TRAVERSE_MISS) return retval;
   }
   return TRAVERSE_MISS;
}

uchar send_event_to_region(LGRegion* r, uiEvent* ev)
{
   // Spew(DSRC_UI_Dispatch,("send_event_to_region(%x,%x)\n",r,ev));
   return ui_traverse_point(r,ev->pos,ev) == TRAVERSE_HIT;
}

uchar uiDispatchEventToRegion(uiEvent* ev, LGRegion* reg)
{
   LGPoint pos;
   uiEvent nev = *ev;
   
   ui_mouse_do_conversion(&(nev.pos.x),&(nev.pos.y),TRUE);
   pos = nev.pos;
   pos.x += reg->r->ul.x - reg->abs_x;
   pos.y += reg->r->ul.y - reg->abs_y;
   
   if (!RECT_TEST_PT(reg->r,pos))
   {
      LGRect r;        
      r.ul = nev.pos;
      r.lr.x = nev.pos.x+1;
      r.lr.y = nev.pos.y+1;
      return event_dispatch_callback(reg,&r,&nev);
   }
   return ui_traverse_point(reg,nev.pos,&nev) == TRAVERSE_HIT;
}


uchar uiDispatchEvent(uiEvent* ev)
{
   int i;
   // Spew(DSRC_UI_Dispatch,("dispatch_event(%x), CurFocus = %d\n",ev,CurFocus));
   if (!(ev->type & uiGlobalEventMask)) return FALSE;
   for (i = CurFocus; i != CHAIN_END; i = FCHAIN[i].next)
   {
      // Spew(DSRC_UI_Dispatch,("dispatch_event(): checking focus chain element %d\n",i));
      if (FCHAIN[i].evmask & ev->type)
         if (uiDispatchEventToRegion(ev,FCHAIN[i].reg)) return TRUE;
   }
   return FALSE;                                                                           
}

errtype uiQueueEvent(uiEvent* ev)
{
   // if this is a keyboard event, queue up earlier events.
   if (ev->type == UI_EVENT_KBD_RAW || ev->type == UI_EVENT_KBD_COOKED)
   {
      kbs_event kbe;
      for(kbe = kb_next(); kbe.code != KBC_NONE; kbe = kb_next())
      {
         uiRawKeyEvent out;
         mouse_get_xy(&out.pos.x,&out.pos.y);
         out.scancode = kbe.code;
         out.action = kbe.state;
         out.type = UI_EVENT_KBD_RAW;
         event_queue_add((uiEvent*)&out);
      }
   }
   if (ev->type == UI_EVENT_MOUSE || ev->type == UI_EVENT_MOUSE_MOVE)
   {
      mouse_event mse;
      errtype err = mouse_next(&mse);
      for(;err == OK; err = mouse_next(&mse))
      {
         uiMouseEvent out;
         out.pos.x = mse.x;
         out.pos.y = mse.y;
         out.type = (mse.type == MOUSE_MOTION) ? UI_EVENT_MOUSE_MOVE :  UI_EVENT_MOUSE;
         out.action = mse.type;
         out.modifiers = mse.modifiers;
         event_queue_add((uiEvent*)&out);
      }
   }
   event_queue_add(ev);
   return OK;
}

#define MOUSE_EVENT_FLUSHED UI_EVENT_MOUSE_MOVE 

void ui_purge_mouse_events(void)
{
   int i;
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      last_down_events[i].type = UI_EVENT_NULL;
      last_down_events[i].tstamp = 0;
      last_up_events[i].type = UI_EVENT_NULL;
      last_up_events[i].tstamp = 0;
   }
}

void ui_flush_mouse_events(ulong timestamp, LGPoint pos)
{
   int i;
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      
      if (uiDoubleClicksOn[i] &&
         last_down_events[i].type != UI_EVENT_NULL)
      {
         int crit = uiDoubleClickDelay;
         ulong timediff = timestamp - last_down_events[i].tstamp;
         LGPoint downpos = last_down_events[i].pos;
         uchar out = (abs(pos.x - downpos.x) > uiDoubleClickTolerance ||
                     abs(pos.y - downpos.y) > uiDoubleClickTolerance);

         printf("Flushing mouse event!\n");

         // OK, if we've waited DoubleClickDelay after a down event, send it out.
         if (out || timediff >= crit)
         {
            printf("flushing old clicks: crit = %d timediff = %d\n",crit,timediff);

            uiMouseEvent ev;
            //Spew(DSRC_UI_Polling,("flushing old clicks: crit = %d timediff = %d\n",crit,timediff));
            if (last_down_events[i].type != MOUSE_EVENT_FLUSHED)
            {
               ev = last_down_events[i];
               last_down_events[i].type = MOUSE_EVENT_FLUSHED;
               uiDispatchEvent((uiEvent*)&ev);
            }
            if (last_up_events[i].type != MOUSE_EVENT_FLUSHED)
            {
               ev = last_up_events[i];
               last_up_events[i].type = MOUSE_EVENT_FLUSHED;
               uiDispatchEvent((uiEvent*)&ev);
            }
         }
         else {
            printf("Nothing to flush!\n");
         }

         // This is where we do our flushing
         if (last_up_events[i].type != UI_EVENT_NULL)
         {
            crit = uiDoubleClickTime;
            timediff = timestamp - last_up_events[i].tstamp;
            if (out || timediff >= crit)
            {
               last_down_events[i].type   = UI_EVENT_NULL;
                 last_up_events[i].type   = UI_EVENT_NULL;
            }
         }
      }
   }
}

void ui_dispatch_mouse_event(uiMouseEvent* mout)
{
   int i;
   uchar eaten = FALSE;

   bool altDown = (SDL_GetModState() & KMOD_ALT) != 0;

   printf("----- ui_dispatch_mouse_event -----\n");
//   ui_mouse_do_conversion(&(mout->pos.x),&(mout->pos.y),TRUE);
   ui_flush_mouse_events(mout->tstamp,mout->pos);
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
   {
      if (!(uiDoubleClicksOn[i]))
         continue;

      printf("Checking double click! %i\n", mout->action & MOUSE_BTN2DOWN(i));
      if (uiAltDoubleClick && altDown)
      {
         if (mout->action & MOUSE_BTN2DOWN(i))
         {
            mout->action &= ~MOUSE_BTN2DOWN(i);
            mout->action |= UI_MOUSE_BTN2DOUBLE(i);
            continue;
         }
      }
      if (last_down_events[i].type != UI_EVENT_NULL)
      {
         printf("Last down event was not null\n");
         if (mout->action & MOUSE_BTN2DOWN(i))
         {
            printf("Double click!\n");
            // Spew(DSRC_UI_Polling,("double click down\n"));
            // make a double click event. 
            mout->action &= ~MOUSE_BTN2DOWN(i);
            mout->action |= UI_MOUSE_BTN2DOUBLE(i);

            last_down_events[i].type = UI_EVENT_NULL;
            last_up_events[i].type = UI_EVENT_NULL;
         }
         if (mout->action & MOUSE_BTN2UP(i))
         {
            printf("Setting up event\n");
            // Spew(DSRC_UI_Polling,("up in time %d\n",mout->tstamp - last_down_events[i].tstamp));
            last_up_events[i] = *mout;
            eaten = TRUE;
         }
      }
      else if (mout->action & MOUSE_BTN2DOWN(i))
      {
         printf("Saving last down event at %i\n", mout->tstamp);
         // Spew(DSRC_UI_Polling,("saving the down\n"));
         last_down_events[i] = *mout;
         eaten = TRUE;
      }
   }
   if (!eaten)
      uiDispatchEvent((uiEvent*)mout);
}

// ----------------------
// KEYBOARD POLLING SETUP
// ----------------------

uchar* ui_poll_keys = NULL;

errtype uiSetKeyboardPolling(ubyte* codes)
{
   ui_poll_keys = codes;
   return OK;
}

static SDL_Scancode sdlScanCodeForSShockKey(uchar key)
{
// from movekeys.c
// #define _Q_			0x0C // q
// #define _W_			0x0D // w
// #define _E_			0x0E // e
// #define _D_			0x02 // d
// #define _A_			0x00 // a
// #define _S_ 			0x01 // s
// #define _Z_			0x06 // z
// #define _X_			0x07 // x
// #define _C_			0x08 // c
// #define _UP_			0x7E // uparrow
// #define _LEFT_		0x7B // leftarrow
// #define _DOWN_	0x7D // downarrow
// #define _RIGHT_	0x7C // rightarrow
// #define _PGUP2_	0x5C // keypad pgup
// #define _HOME2_	0x59 // keypad home
// #define _END2_		0x53 // keypad End
// #define _PGDN2_	0x55 // keypad pgdn
// #define _PAD5_		0x57 // keypad 5
// #define _UP2_		0x5B // second uparrow
// #define _LEFT2_ 	0x56 // second leftarrow
// #define _DOWN2_	0x54 // second downarrow
// #define _RIGHT2_	0x58 // second rightarrow
// #define _SPACE_	0x31 // spacebar
// #define _ENTER2_	0x4C // keypad enter
// #define _ENTER_	0x24 // enter
// #define _R_			0x0F // r
// #define _V_			0x09 // v
// #define _J_			0x26 // j

	SDL_Keycode kc = 0;

	switch(key)
	{
		// TODO: hacky, incomplete, etc
		case 0x0C : kc = SDLK_q ; break;
		case 0x0D : kc = SDLK_w ; break;
		case 0x0E : kc = SDLK_e ; break;
		case 0x02 : kc = SDLK_d ; break;
		case 0x00 : kc = SDLK_a ; break;
		case 0x01 : kc = SDLK_s ; break;
		case 0x06 : kc = SDLK_z ; break;
		case 0x07 : kc = SDLK_x ; break;
		case 0x08 : kc = SDLK_c ; break;
		case 0x7E : kc = SDLK_UP ; break;
		case 0x7B : kc = SDLK_LEFT ; break;
		case 0x7C : kc = SDLK_RIGHT ; break;
		case 0x7D : kc = SDLK_DOWN ; break;
		case 0x5C : kc = SDLK_KP_9 ; break;
		case 0x59 : kc = SDLK_KP_7 ; break;
		case 0x53 : kc = SDLK_KP_1 ; break;
		case 0x55 : kc = SDLK_KP_3 ; break;
		case 0x57 : kc = SDLK_KP_5 ; break;
		case 0x5B : kc = SDLK_KP_8 ; break;
		case 0x56 : kc = SDLK_KP_4 ; break;
		case 0x54 : kc = SDLK_KP_2 ; break;
		case 0x58 : kc = SDLK_KP_6 ; break;
		case 0x31 : kc = SDLK_SPACE ; break;
		case 0x4C : kc = SDLK_KP_ENTER ; break;
		case 0x24 : kc = SDLK_RETURN ; break;
		case 0x0F : kc = SDLK_r ; break;
		case 0x09 : kc = SDLK_v ; break;
		case 0x26 : kc = SDLK_j ; break;
		//case 0x : kc = SDLK_ ; break;

	}

	return SDL_GetScancodeFromKey(kc);
}

// KLC - For Mac version, call GetKeys once at the beginning, then check
// the results in the loop.  Fill in the "mods" field (ready for cooking)
// before dispatching an event.
void ui_poll_keyboard(void)
{
#if 1

	SDL_PumpEvents();

	int numKeys = 0;
	const Uint8* keys = SDL_GetKeyboardState(&numKeys);
	ushort mods = 0;
	SDL_Keymod km = SDL_GetModState();
	if(km & KMOD_CTRL)
		mods |= KB_FLAG_CTRL;
	if(km & KMOD_SHIFT)
		mods |= KB_FLAG_SHIFT;
	// TODO: what's 0x04 ? windows key?
	if(km & KMOD_ALT)
		mods |= KB_FLAG_ALT;

	// FIXME: this is fugly, maybe we could get the state from kbMac.c
	//        like a char[255] there with the state of the keys in sshock codes
	for(uchar* key = ui_poll_keys; *key != KBC_NONE; key++)
	{
		SDL_Scancode sc = sdlScanCodeForSShockKey(*key);
		if(sc != 0 && keys[sc])
		{
			uiPollKeyEvent ev;
			ev.type = UI_EVENT_KBD_POLL;
			ev.action = KBS_DOWN;
			ev.scancode = *key;
			ev.mods = mods;

			printf("\n########### %s generating event with key %x\n", __func__, (int)*key);

			uiDispatchEvent((uiEvent*)&ev);
		}
		// *key is a System Shock/Mac keycode
	}

#else
	extern uchar	pKbdGetKeys[16];
	long			*keys = (long *)pKbdGetKeys;
	GetKeys((UInt32 *)keys);

	uchar *key;
	for (key = ui_poll_keys; *key != KBC_NONE; key++)
		if((pKbdGetKeys[*key>>3] >> (*key & 7)) & 1)
		{
			uiPollKeyEvent ev;
			ev.type = UI_EVENT_KBD_POLL;
			ev.action = KBS_DOWN;
			ev.scancode = *key;
			ev.mods = 0;
			if ((keys[1] & 0x00000001) != 0L)	// Shift key
				ev.mods |= KB_FLAG_SHIFT;
			if ((keys[1] & 0x00008000) != 0L)	// Cmd key
				ev.mods |= KB_FLAG_CTRL;
			if ((keys[1] & 0x00000004) != 0L)	// Option key
				ev.mods |= KB_FLAG_ALT;

			uiDispatchEvent((uiEvent*)&ev);
		}
#endif
}

void ui_pop_up_keys(void)
{
/*¥¥¥ serve any purpose now?
   if (ui_poll_keys != NULL)
   {
      uchar* key;
      for (key = ui_poll_keys; *key != KBC_NONE; key++)
      {
         kb_clear_state(*key,KBA_STATE);
      }
   }
*/
}

errtype uiMakeMotionEvent(uiMouseEvent* ev)
{
   // haha, this is the super secret mouse library variable of the 
   // current button state. 
   extern short mouseInstantButts;
   mouse_get_xy(&ev->pos.x,&ev->pos.y);
   ev->type = UI_EVENT_MOUSE_MOVE; // must get past event mask
   ev->action = MOUSE_MOTION;
   ev->tstamp = mouse_get_time();
   ev->buttons = (ubyte)mouseInstantButts;
   return OK;
}

// Generate a fake mouse motion event and send it along...
LGPoint ui_poll_mouse_position(void)
{
   uiMouseEvent ev;
   uiMakeMotionEvent(&ev);
   uiDispatchEvent((uiEvent *)(&ev));
   return ev.pos;
}


errtype uiPoll(void)
{
   static LGPoint last_mouse = { -1, -1 };
   errtype err;
   uiEvent out,*ev;
   uchar kbdone = FALSE;
   uchar msdone = FALSE;
   LGPoint mousepos = last_mouse;
   extern LGPoint LastCursorPos;
   extern struct _cursor* LastCursor;
   extern void ui_update_cursor(LGPoint pos);

#define BURN_QUEUE
#ifdef BURN_QUEUE
   // burn through queue
   while(event_queue_next(&ev))
   {
      uchar result = TRUE;
//      ui_mouse_do_conversion(&(ev->pos.x),&(ev->pos.y),TRUE);
      if (ev->type == UI_EVENT_MOUSE)
         ui_dispatch_mouse_event((uiMouseEvent*)ev);
      else result = uiDispatchEvent(ev);
      if (!result && ev->type == UI_EVENT_KBD_RAW)
      {
         ushort cooked;
         kbs_event kbe;
         kbe.code = ((uiRawKeyEvent*)ev)->scancode;
         kbe.state = ((uiRawKeyEvent*)ev)->action;
         err = kb_cook(kbe,&cooked,&result);
         if (err != OK) return err;
         if (result)
         {
            out.subtype = cooked;
            out.type = UI_EVENT_KBD_COOKED;
            uiDispatchEvent(ev);
         }
      }
   }
#endif // BURN_QUEUE

//   ui_mouse_get_xy(&mousepos.x,&mousepos.y);

   printf("mouse_get_xy\n");
   mouse_get_xy(&mousepos.x,&mousepos.y);

   while(!kbdone || !msdone)
   {
      if (!kbdone)
      {
         kbs_event kbe = kb_next();
         if (kbe.code != KBC_NONE)
         {
            uchar eaten;
            uiRawKeyEvent* ev = (uiRawKeyEvent*)&out;
            // Spew(DSRC_UI_Polling,("uiPoll(): got a keyboard event: <%d,%x>\n",kbe.state,kbe.code));
            ev->pos = mousepos;
            ev->scancode = kbe.code;
            ev->action = kbe.state;
            ev->type = UI_EVENT_KBD_RAW;
            eaten = uiDispatchEvent((uiEvent*)ev);
            if (!eaten)
            {
              ushort cooked;
              uchar result;
              // Spew(DSRC_UI_Polling,("uiPoll(): cooking keyboard event: <%d,%x>\n",kbe.state,kbe.code));
              err = kb_cook(kbe,&cooked,&result);
              if (err != OK) return err;
              if (result)
              {
               out.subtype = cooked;
               out.type = UI_EVENT_KBD_COOKED;
               eaten = uiDispatchEvent(&out);
              }
            }
//            if (eaten)
//            {
//               kb_clear_state(kbe.code,KBA_STATE);
//            }
         }
         else kbdone = TRUE;
      }
      if (!msdone)
      {
         printf(" !msdone\n");
         mouse_event mse;
         errtype err = mouse_next(&mse);
         /*if (poll_mouse_motion)
            while (mse.type == MOUSE_MOTION  && err == OK)
            {
               err = mouse_next(&mse);
            }*/
         if (err == OK)
         {
            uiMouseEvent* mout = (uiMouseEvent*)&out;
            out.pos.x = mse.x;
            out.pos.y = mse.y;
            // note that the equality operator here means that motion-only 
            // events are MOUSE_MOVE, and others are MOUSE events.
            out.type = (mse.type == MOUSE_MOTION) ? UI_EVENT_MOUSE_MOVE :  UI_EVENT_MOUSE;
            out.subtype = mse.type;
            mout->tstamp = mse.timestamp;
            mout->buttons = mse.buttons;
         	mout->modifiers = mse.modifiers;
            ui_dispatch_mouse_event(mout);
//            uiDispatchEvent((uiEvent*)mout);
         }
         else msdone = TRUE;
      }
   }

   if (poll_mouse_motion)
   {
      mousepos = ui_poll_mouse_position();
   }
   if (ui_poll_keys != NULL && (uiGlobalEventMask & UI_EVENT_KBD_POLL))
      ui_poll_keyboard();
   ui_flush_mouse_events(mouse_get_time(),mousepos);
   if (!PointsEqual(mousepos,last_mouse))
   {
      ui_update_cursor(mousepos);
      last_mouse = mousepos;
   }

   return OK;
}

errtype uiSetMouseMotionPolling(uchar poll)
{
   if (poll) mouseMask &= ~MOUSE_MOTION;
   else mouseMask |= MOUSE_MOTION;
   poll_mouse_motion = poll;
   return OK;
}



errtype uiFlush(void)
{
   uiEvent* e;
   kbs_event kbe = kb_next();
   mouse_flush();
   
   while (kbe.code != KBC_NONE)
   {
      ushort dummy;
      uchar result;
      kb_cook(kbe,&dummy,&result);
      kbe = kb_next();
   }
   while(event_queue_next(&e));
   ui_pop_up_keys();
   ui_purge_mouse_events();
   return OK;
}

uchar uiCheckInput(void)
{
   kbs_event kbe;
   mouse_event mse;
   kbe = kb_next();
   if (kbe.code != KBC_NONE)
   {
      ushort cooked;
      uchar res;
      kb_cook(kbe,&cooked,&res);
      if (kbe.state == KBS_DOWN)
      {
         ui_pop_up_keys();
         return TRUE;
      }
   }
   if (mouse_next(&mse) == OK)
   {
      int i;
      for (i = 0; i < NUM_MOUSE_BTNS; i++)
      {
         if ((mse.type & MOUSE_BTN2DOWN(i) ) != 0)
            return TRUE;
      }
   }
   return FALSE;
}

// ---------------------------
// INITIALIZATION AND SHUTDOWN
// ---------------------------

//char keybuf[512];

errtype uiInit(uiSlab* slab)
{
   int i;
   errtype err;

   uiSetCurrentSlab(slab);
//KLC - moved to main program   mouse_init(grd_cap->w,grd_cap->h);
//KLC - moved to main program   kb_init(NULL);
   // initialize the event queue;
   EventQueue.in = EventQueue.out = 0;
   EventQueue.size = INITIAL_QUEUE_SIZE;
   EventQueue.vec = (uiEvent *)malloc(sizeof(uiEvent)*INITIAL_QUEUE_SIZE);
   for (i = 0; i < NUM_MOUSE_BTNS; i++)
      last_down_events[i].type = UI_EVENT_NULL;
//KLC - done in main program now.   err = ui_init_cursors();
   if (err != OK) return err;
//KLC -   AtExit(uiShutdown);
   return OK;
}

void uiShutdown(void)
{
   extern errtype ui_shutdown_cursors(void);
   ui_shutdown_cursors();
   mouse_shutdown();
   kb_close();
}


errtype uiShutdownRegionHandlers(LGRegion* r)
{
   errtype err = OK;
   handler_chain *ch = (handler_chain*)(r->handler);
   if (ch == NULL) return ERR_NOEFFECT;
   err = array_destroy(&ch->chain);
   free(ch);
   return err;
}

errtype ui_init_focus_chain(uiSlab* slab)
{
   errtype err = array_init(&slab->fchain.chain,sizeof(focus_link),INITIAL_FOCUSES);
   if (err != OK)  return err;
   slab->fchain.curfocus = CHAIN_END;
   return OK;
}

      











