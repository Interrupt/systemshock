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

//
// DG 2018: (eventually) SDL versions of the functions previously in kbMac.c, mouse.c and kbcook.c
//

#include "lg.h"
#include "kb.h"
#include "mouse.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <OpenGL.h>

extern SDL_Window *window;
extern SDL_Renderer *renderer;

static bool fullscreenActive = false;

static void toggleFullScreen()
{
	fullscreenActive = !fullscreenActive;
	SDL_SetWindowFullscreen(window, fullscreenActive ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

// current state of the keys, based on the SystemShock/Mac Keycodes (sshockKeyStates[keyCode] has the state for that key)
// set at the beginning of each frame in pump_events()
uchar sshockKeyStates[256];

enum {
	kNumKBevents = 128,
	kNumMouseEvents = 128
};

// queue keyboard events, created in pump_events(), consumed by kb_next()
static kbs_event kbEvents[kNumKBevents];
static int nextKBevent = 0; // where next to insert (also, if 0 there are no events)

static void addKBevent(const kbs_event* ev)
{
	if(nextKBevent < kNumKBevents)
	{
		kbEvents[nextKBevent] = *ev;
		++nextKBevent;
	}
	else
	{
		printf("WTF, the kbEvents queue is full?!");
		// drop the oldest event
		memmove(&kbEvents[0], &kbEvents[1], sizeof(kbs_event)*(kNumKBevents-1));
		kbEvents[kNumKBevents-1] = *ev;
	}
}

// same for mouse events, also created in pump_events(), consumed by mouse_next()
static mouse_event mouseEvents[kNumMouseEvents];
static int nextMouseEvent = 0;

// latest mouse state as input for MousePollProc() in mouse.c
mouse_event latestMouseEvent;

static void addMouseEvent(const mouse_event* ev)
{
	latestMouseEvent = *ev;

	// Fill out the buttons
	uint mouse_state = SDL_GetMouseState(NULL, NULL);
	latestMouseEvent.buttons = 0;
	if (mouse_state & SDL_BUTTON_LMASK)
		latestMouseEvent.buttons |= (1 << MOUSE_LBUTTON);
	if (mouse_state & SDL_BUTTON_RMASK)
		latestMouseEvent.buttons |= (1 << MOUSE_RBUTTON);

	// confine the mouse coordinates to the game window
	int width, height;
	SDL_RenderGetLogicalSize(renderer, &width, &height);

	if (latestMouseEvent.x < 0) latestMouseEvent.x = 0;
	if (latestMouseEvent.x >= width) latestMouseEvent.x = width - 1;
	if (latestMouseEvent.y < 0) latestMouseEvent.y = 0;
	if (latestMouseEvent.y >= height) latestMouseEvent.y = height - 1;

	// in fullscreen mode, do not allow the mouse to leave the window; might
	// be smarter to use SDL relative mouse mode instead
	if (fullscreenActive && (latestMouseEvent.x != ev->x || latestMouseEvent.y != ev->y)) {
		mouse_put_xy(latestMouseEvent.x, latestMouseEvent.y);
	}

	if(nextMouseEvent < kNumMouseEvents)
	{
		mouseEvents[nextMouseEvent] = latestMouseEvent;
		++nextMouseEvent;
	}
	else
	{
		printf("WTF, the mouseEvents queue is full?!");
		// drop the oldest event
		memmove(&mouseEvents[0], &mouseEvents[1], sizeof(mouse_event)*(kNumMouseEvents-1));
		mouseEvents[kNumMouseEvents-1] = latestMouseEvent;
	}
}

static uchar sdlKeyCodeToSSHOCKkeyCode(SDL_Keycode kc)
{
	// apparently System Shock uses the same keycodes as Mac
	// which are luckily documented, see
	// see http://snipplr.com/view/42797/
	// and https://stackoverflow.com/a/16125341
	// see also GameSrc/movekeys.c for a very short list

	//printf("sdlKeyCodeToSSHOCKkeyCode: %x\n", kc);

	switch(kc)
	{
		case SDLK_a : return 0x00; //  kVK_ANSI_A = 0x00,
		case SDLK_s : return 0x01; //  kVK_ANSI_S = 0x01,
		case SDLK_d : return 0x02; //  kVK_ANSI_D = 0x02,
		case SDLK_f : return 0x03; //  kVK_ANSI_F = 0x03,
		case SDLK_h : return 0x04; //  kVK_ANSI_H = 0x04,
		case SDLK_g : return 0x05; //  kVK_ANSI_G = 0x05,
		case SDLK_z : return 0x06; //  kVK_ANSI_Z = 0x06,
		case SDLK_x : return 0x07; //  kVK_ANSI_X = 0x07,
		case SDLK_c : return 0x08; //  kVK_ANSI_C = 0x08,
		case SDLK_v : return 0x09; //  kVK_ANSI_V = 0x09,
		case SDLK_b : return 0x0B; //  kVK_ANSI_B = 0x0B,
		case SDLK_q : return 0x0C; //  kVK_ANSI_Q = 0x0C,
		case SDLK_w : return 0x0D; //  kVK_ANSI_W = 0x0D,
		case SDLK_e : return 0x0E; //  kVK_ANSI_E = 0x0E,
		case SDLK_r : return 0x0F; //  kVK_ANSI_R = 0x0F,
		case SDLK_y : return 0x10; //  kVK_ANSI_Y = 0x10,
		case SDLK_t : return 0x11; //  kVK_ANSI_T = 0x11,
		case SDLK_1 : return 0x12; //  kVK_ANSI_1 = 0x12,
		case SDLK_2 : return 0x13; //  kVK_ANSI_2 = 0x13,
		case SDLK_3 : return 0x14; //  kVK_ANSI_3 = 0x14,
		case SDLK_4 : return 0x15; //  kVK_ANSI_4 = 0x15,
		case SDLK_6 : return 0x16; //  kVK_ANSI_6 = 0x16,
		case SDLK_5 : return 0x17; //  kVK_ANSI_5 = 0x17,
		case SDLK_EQUALS : return 0x18; //  kVK_ANSI_Equal = 0x18,
		case SDLK_9 : return 0x19; //  kVK_ANSI_9 = 0x19,
		case SDLK_7 : return 0x1A; //  kVK_ANSI_7 = 0x1A,
		case SDLK_MINUS : return 0x1B; //  kVK_ANSI_Minus = 0x1B,
		case SDLK_8 : return 0x1C; //  kVK_ANSI_8 = 0x1C,
		case SDLK_0 : return 0x1D; //  kVK_ANSI_0 = 0x1D,
		case SDLK_RIGHTBRACKET : return 0x1E; //  kVK_ANSI_RightBracket = 0x1E,
		case SDLK_o : return 0x1F; //  kVK_ANSI_O = 0x1F,
		case SDLK_u : return 0x20; //  kVK_ANSI_U = 0x20,
		case SDLK_LEFTBRACKET : return 0x21; //  kVK_ANSI_LeftBracket = 0x21,
		case SDLK_i : return 0x22; //  kVK_ANSI_I = 0x22,
		case SDLK_p : return 0x23; //  kVK_ANSI_P = 0x23,
		case SDLK_l : return 0x25; //  kVK_ANSI_L = 0x25,
		case SDLK_j : return 0x26; //  kVK_ANSI_J = 0x26,
		case SDLK_QUOTE : return 0x27; //  kVK_ANSI_Quote = 0x27, // TODO: or QUOTEDBL ?
		case SDLK_k : return 0x28; //  kVK_ANSI_K = 0x28,
		case SDLK_SEMICOLON : return 0x29; //  kVK_ANSI_Semicolon = 0x29,
		case SDLK_BACKSLASH : return 0x2A; //  kVK_ANSI_Backslash = 0x2A,
		case SDLK_COMMA : return 0x2B; //  kVK_ANSI_Comma = 0x2B,
		case SDLK_SLASH : return 0x2C; //  kVK_ANSI_Slash = 0x2C,
		case SDLK_n : return 0x2D; //  kVK_ANSI_N = 0x2D,
		case SDLK_m : return 0x2E; //  kVK_ANSI_M = 0x2E,
		case SDLK_PERIOD : return 0x2F; //  kVK_ANSI_Period = 0x2F,
		case SDLK_BACKQUOTE : return 0x32; //  kVK_ANSI_Grave = 0x32, // TODO: really?
		case SDLK_KP_DECIMAL : return 0x41; //  kVK_ANSI_KeypadDecimal   = 0x41,
		case SDLK_KP_MULTIPLY : return 0x43; //  kVK_ANSI_KeypadMultiply = 0x43,
		case SDLK_KP_PLUS : return 0x45; //  kVK_ANSI_KeypadPlus = 0x45,
		case SDLK_KP_CLEAR : return 0x47; //  kVK_ANSI_KeypadClear = 0x47,
		case SDLK_KP_DIVIDE : return 0x4B; //  kVK_ANSI_KeypadDivide = 0x4B,
		case SDLK_KP_ENTER : return 0x4C; //  kVK_ANSI_KeypadEnter   = 0x4C, aka _ENTER2_
		case SDLK_KP_MINUS : return 0x4E; //  kVK_ANSI_KeypadMinus   = 0x4E,
		case SDLK_KP_EQUALS : return 0x51; //  kVK_ANSI_KeypadEquals = 0x51,
		case SDLK_KP_0 : return 0x52; //  kVK_ANSI_Keypad0 = 0x52,
		case SDLK_KP_1 : return 0x53; //  kVK_ANSI_Keypad1 = 0x53, aka _END2_
		case SDLK_KP_2 : return 0x54; //  kVK_ANSI_Keypad2 = 0x54, aka _DOWN2_
		case SDLK_KP_3 : return 0x55; //  kVK_ANSI_Keypad3 = 0x55, aka _PGDN2_
		case SDLK_KP_4 : return 0x56; //  kVK_ANSI_Keypad4 = 0x56, aka _LEFT2_
		case SDLK_KP_5 : return 0x57; //  kVK_ANSI_Keypad5 = 0x57, aka _PAD5_
		case SDLK_KP_6 : return 0x58; //  kVK_ANSI_Keypad6 = 0x58, aka _RIGHT2_
		case SDLK_KP_7 : return 0x59; //  kVK_ANSI_Keypad7 = 0x59, aka _HOME2_
		case SDLK_KP_8 : return 0x5B; //  kVK_ANSI_Keypad8 = 0x5B, aka _UP2_
		case SDLK_KP_9 : return 0x5C; //  kVK_ANSI_Keypad9 = 0x5C, aka _PGUP2_

		// keycodes for keys that are independent of keyboard layout*/
		case SDLK_RETURN : return 0x24; //  kVK_Return  = 0x24,
		case SDLK_TAB : return 0x30; //  kVK_Tab     = 0x30,
		case SDLK_SPACE : return 0x31; //  kVK_Space   = 0x31,
		case SDLK_DELETE : return 0x33; //  kVK_Delete  = 0x33,
		case SDLK_BACKSPACE : return 0x33; //  kVK_Delete  = 0x33,
		case SDLK_ESCAPE : return 0x35; //  kVK_Escape  = 0x35,
		case SDLK_LGUI : // fall-through
		case SDLK_RGUI : return 0x37; //  kVK_Command = 0x37, // FIXME: I think command is the windows/meta key?
		case SDLK_LSHIFT : return 0x38; //  kVK_Shift   = 0x38,
		case SDLK_CAPSLOCK : return 0x39; //  kVK_CapsLock= 0x39,
		case SDLK_LALT : return 0x3A; //  kVK_Option  = 0x3A, Option == Aalt
		case SDLK_LCTRL : return 0x3B; //  kVK_Control = 0x3B,
		case SDLK_RSHIFT : return 0x3C; //  kVK_RightShift  = 0x3C,
		case SDLK_RALT : return 0x3D; //  kVK_RightOption = 0x3D,
		case SDLK_RCTRL : return 0x3E; //  kVK_RightControl = 0x3E,
		//case SDLK_ : return 0x3F; //  kVK_Function = 0x3F, // TODO: what's this?
		case SDLK_F17 : return 0x40; //  kVK_F17 = 0x40,
		case SDLK_VOLUMEUP : return 0x48; //  kVK_VolumeUp = 0x48,
		case SDLK_VOLUMEDOWN : return 0x49; //  kVK_VolumeDown = 0x49,
		case SDLK_MUTE : return 0x4A; //  kVK_Mute = 0x4A,
		case SDLK_F18 : return 0x4F; //  kVK_F18 = 0x4F,
		case SDLK_F19 : return 0x50; //  kVK_F19 = 0x50,
		case SDLK_F20 : return 0x5A; //  kVK_F20 = 0x5A,
		case SDLK_F5  : return 0x60; //  kVK_F5  = 0x60,
		case SDLK_F6  : return 0x61; //  kVK_F6  = 0x61,
		case SDLK_F7  : return 0x62; //  kVK_F7  = 0x62,
		case SDLK_F3  : return 0x63; //  kVK_F3  = 0x63,
		case SDLK_F8  : return 0x64; //  kVK_F8  = 0x64,
		case SDLK_F9  : return 0x65; //  kVK_F9  = 0x65,
		case SDLK_F11 : return 0x67; //  kVK_F11 = 0x67,
		case SDLK_F13 : return 0x69; //  kVK_F13 = 0x69,
		case SDLK_F16 : return 0x6A; //  kVK_F16 = 0x6A,
		case SDLK_F14 : return 0x6B; //  kVK_F14 = 0x6B,
		case SDLK_F10 : return 0x6D; //  kVK_F10 = 0x6D,
		case SDLK_F12 : return 0x6F; //  kVK_F12 = 0x6F,
		case SDLK_F15 : return 0x71; //  kVK_F15 = 0x71,
		case SDLK_HELP : return 0x72; //  kVK_Help = 0x72,
		case SDLK_HOME : return 0x73; //  kVK_Home = 0x73,
		case SDLK_PAGEUP : return 0x74; //  kVK_PageUp = 0x74,
		//case SDLK_ : return 0x75; //  kVK_ForwardDelete = 0x75, // TODO: what's this?
		case SDLK_F4 : return 0x76; //  kVK_F4 = 0x76,
		case SDLK_END : return 0x77; //  kVK_End = 0x77,
		case SDLK_F2 : return 0x78; //  kVK_F2 = 0x78,
		case SDLK_PAGEDOWN : return 0x79; //  kVK_PageDown = 0x79,
		case SDLK_F1 : return 0x7A; //  kVK_F1 = 0x7A,
		case SDLK_LEFT : return 0x7B;  //  kVK_LeftArrow  = 0x7B, aka _LEFT_
		case SDLK_RIGHT : return 0x7C; //  kVK_RightArrow = 0x7C, aka _RIGHT
		case SDLK_DOWN : return 0x7D;  //  kVK_DownArrow  = 0x7D, aka _DOWN_
		case SDLK_UP : return 0x7E;    //  kVK_UpArrow    = 0x7E, aka _UP_
	}

	return KBC_NONE;
}

void pump_events(void)
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_QUIT:
				// a bit hacky at this place, but this would allow exiting the game via the window's [x] button
				exit(0); // TODO: I guess there is a better way.
				break;


			// TODO: really also handle key up here? the mac code apparently didn't, but where else do
			//       kbs_events with .state == KBS_UP come from?
			case SDL_KEYUP:
			case SDL_KEYDOWN:
			{
				uchar c = sdlKeyCodeToSSHOCKkeyCode(ev.key.keysym.sym);
				if(c != KBC_NONE)
				{
					kbs_event keyEvent = { 0 };
					keyEvent.code = c;

					//printf("ev.key.keysym.sym: %x\n", ev.key.keysym.sym);

					// FIXME: this is hacky, see comment in SDL_TEXTINPUT below..
					if(ev.key.keysym.sym >= 0x08 && ev.key.keysym.sym <= '~')
						keyEvent.ascii = ev.key.keysym.sym;
					else
						keyEvent.ascii = 0;

					keyEvent.modifiers = 0;
					Uint16 mod = ev.key.keysym.mod;
					if(mod & KMOD_CTRL)
						keyEvent.modifiers |= KB_MOD_CTRL;
					if(mod & KMOD_SHIFT)
						keyEvent.modifiers |= KB_MOD_SHIFT;
					// TODO: what's 0x04 ? windows key?
					if(mod & KMOD_ALT)
						keyEvent.modifiers |= KB_MOD_ALT;

					if(ev.key.state == SDL_PRESSED)
					{
						if (ev.key.keysym.sym == SDLK_RETURN && mod & KMOD_ALT) {
							toggleFullScreen();
							break;
						}
						keyEvent.state = KBS_DOWN;
						sshockKeyStates[c] = keyEvent.modifiers | KB_MOD_PRESSED;
						addKBevent(&keyEvent);
					}
					else
					{
						sshockKeyStates[c] = 0;
						keyEvent.state = KBS_UP;
						addKBevent(&keyEvent);
					}
				}

				break;
			}
			case SDL_TEXTINPUT:
				// ev.text.text is a char[32] UTF-8 string
				// TODO: this is the proper way to get text input, and the only reliable way
				//       to get the correct char for non-letter-key + shift (e.g. Shift+2 is " on German KB)
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				bool down = (ev.button.state == SDL_PRESSED);
				mouse_event mouseEvent = {0};
				mouseEvent.type = 0;

				switch (ev.button.button)
				{
					case SDL_BUTTON_LEFT:
						// TODO: the old mac code used to emulate right mouse clicks if space, enter, or return
						//       was pressed at the same time - do the same? (=> could check sshockKeyStates[])

						mouseEvent.type = down ? MOUSE_LDOWN : MOUSE_LUP;
						break;

					case SDL_BUTTON_RIGHT:
						mouseEvent.type = down ? MOUSE_RDOWN : MOUSE_RUP;
						break;

					//case SDL_BUTTON_MIDDLE: // TODO: is this MOUSE_CDOWN/UP ?
					//		break;

				}

				if(mouseEvent.type != 0)
				{
					mouseEvent.x = ev.button.x;
					mouseEvent.y = ev.button.y;
					mouseEvent.timestamp = mouse_get_time();
					mouseEvent.modifiers = 0;

					addMouseEvent(&mouseEvent);
				}

				break;
			}
			case SDL_MOUSEMOTION:
			{
				mouse_event mouseEvent = {0};
				mouseEvent.type = MOUSE_MOTION;
				mouseEvent.x = ev.motion.x; // TODO: relative mode?
				mouseEvent.y = ev.motion.y;
				mouseEvent.timestamp = mouse_get_time();

				addMouseEvent(&mouseEvent);
				break;
			}
			case SDL_MOUSEWHEEL:
				if (ev.wheel.y != 0) {
					mouse_event mouseEvent = {0};
					mouseEvent.type = ev.wheel.y < 0 ? MOUSE_WHEELDN : MOUSE_WHEELUP;
					mouseEvent.x = latestMouseEvent.x;
					mouseEvent.y = latestMouseEvent.y;
					mouseEvent.timestamp = mouse_get_time();
					addMouseEvent(&mouseEvent);
				}
				break;
			case SDL_WINDOWEVENT:
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
					opengl_resize(ev.window.data1, ev.window.data2);
			// TODO: maybe handle other events as well..
		}
	}
}


//===============================================================
//
// This section is adapted from:
// kbMac.c - All the keyboard handling routines that are specific to the Macintosh.
//
//===============================================================

//------------------
//  Globals
//------------------
int 		pKbdStatusFlags;
uchar		pKbdGetKeys[16]; // TODO: eliminate, use sshockKeyStates instead

//---------------------------------------------------------------
//  Startup and keyboard handlers and initialize globals.   Shutdown follows.
//---------------------------------------------------------------
int kb_startup(void * v)
{
	pKbdStatusFlags = 0;

	LG_memset(pKbdGetKeys, 0, 16);								// Zero out the GetKeys buffer

	LG_memset(sshockKeyStates, 0, sizeof(sshockKeyStates));
	nextKBevent = 0;

	return (0);
}

int kb_shutdown(void)
{
	return (0);
}

//---------------------------------------------------------------
//  Get and set the global flags.
//---------------------------------------------------------------
int kb_get_flags()
{
	return (pKbdStatusFlags);
}

void kb_set_flags(int flags)
{
	pKbdStatusFlags = flags;
}

//---------------------------------------------------------------
//  Get the next available key from the event queue.
//---------------------------------------------------------------
kbs_event kb_next(void)
{
	kbs_event retEvent = kb_look_next();
	// kb_look_next() doesn't remove events from the queue, this function does,
	// right here (but only if there actually was an event in the queue, of course):
	if(nextKBevent > 0)
	{
		--nextKBevent;
		memmove(&kbEvents[0], &kbEvents[1], sizeof(kbs_event)*(kNumKBevents-1));
	}
	return retEvent;

#if 0
	bool gotKey = FALSE;
	EventRecord	theEvent;
	while(!gotKey)
	{
		gotKey = GetOSEvent(keyDownMask | autoKeyMask, &theEvent);		// Get a key
		if (gotKey)
		{
			retEvent.code = (uchar)(theEvent.message >> 8); // keyCodeMask == 0x0000FF00
			retEvent.state = KBS_DOWN;
			retEvent.ascii = (uchar)(theEvent.message & charCodeMask);
			retEvent.modifiers = (uchar)(theEvent.modifiers >> 8);
		}
		else if ((flags & KBF_BLOCK) == 0)					// If there was no key and we're
			return (retEvent);										// not blocking, then return.
	}
	return (retEvent);
#endif
}

//---------------------------------------------------------------
//  See if there is a key waiting in the queue.
//---------------------------------------------------------------
kbs_event kb_look_next(void)
{
	kbs_event		retEvent = { 0xFF, 0x00 };

	int				flags = kb_get_flags();
	if(flags & KBF_BLOCK)
	{
		while(nextKBevent == 0)
		{
			pump_events();
		}
	}

	if(nextKBevent > 0)
	{
		retEvent = kbEvents[0];
	}
	return retEvent;

#if 0
	bool				gotKey = FALSE;
	EventRecord	theEvent;
	while(!gotKey)
	{
		gotKey = OSEventAvail(keyDownMask | autoKeyMask, &theEvent);		// Get a key
		if (gotKey)
		{
			retEvent.code = (uchar)(theEvent.message >> 8);
			retEvent.state = KBS_DOWN;
			retEvent.ascii = (uchar)(theEvent.message & charCodeMask);
			retEvent.modifiers = (uchar)(theEvent.modifiers >> 8);
		}
		else if (flags & KBF_BLOCK == 0)					// If there was no key and we're
			return (retEvent);										// not blocking, then return.
	}
	return (retEvent);
#endif
}


//---------------------------------------------------------------
//  Flush keyboard events from the event queue.
//---------------------------------------------------------------
void kb_flush(void)
{
	// http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/Event_Manager/event_mgr_ref/function_group_5.html#//apple_ref/c/func/FlushEvents
	//FlushEvents(keyDownMask | autoKeyMask, 0);

	SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP); // Note: that's a range!

	nextKBevent = 0; // this flushes the keyboard events already buffered - TODO is that desirable?
}


//---------------------------------------------------------------
//  Return the state of the indicated key (scan code).
//---------------------------------------------------------------
uchar kb_state(uchar code)
{
	// see http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/Event_Manager/event_mgr_ref/function_group_4.html#//apple_ref/c/func/GetKeys
	//GetKeys((UInt32 *) pKbdGetKeys);
	//return ((pKbdGetKeys[code>>3] >> (code & 7)) & 1);

	return sshockKeyStates[code] != 0;
}


//---------------------------
//
// MOUSE STUFF
//
//---------------------------


// ---------------------------------------------------------
// mouse_next gets the event in the front event queue,
// and removes the event from the queue.
// res = ptr to event to be filled.
//	---------------------------------------------------------
//  For Mac version: Get event from the normal Mac event queue for mouse events.
//  The events looked for depend on the 'mouseMask' setting.

uchar btn_left = FALSE;
uchar btn_right = FALSE;
errtype mouse_next(mouse_event *res)
{
	if(nextMouseEvent <= 0)
		return ERR_DUNDERFLOW;

	*res = mouseEvents[0];

	--nextMouseEvent;
	memmove(&mouseEvents[0], &mouseEvents[1], sizeof(mouse_event)*(kNumMouseEvents-1));

	return OK;
}

errtype mouse_flush(void)
{
	//FlushEvents(mouseDown | mouseUp, 0);
//   Spew(DSRC_MOUSE_Flush,("Entering mouse_flush()\n"));
	//mouseQueueIn = mouseQueueOut = 0;
	nextMouseEvent = 0;
	// TODO: anything else?
	return OK;
}

errtype mouse_get_xy(short* x, short* y)
{
	*x = latestMouseEvent.x;
	*y = latestMouseEvent.y;
	return OK;
}

errtype mouse_put_xy(short x, short y)
{
	latestMouseEvent.x = x;
	latestMouseEvent.y = y;

	// scale new mouse coordinates from logical (game resolution) to
	// physical (SDL window size)

	int physical_width, physical_height;
	SDL_GetWindowSize(window, &physical_width, &physical_height);

	int logical_width, logical_height;
	SDL_RenderGetLogicalSize(renderer, &logical_width, &logical_height);

	float scale_x = (float)physical_width / logical_width;
	float scale_y = (float)physical_height / logical_height;

	if (scale_x >= scale_y) {
		// physical aspect ratio is wider; black borders left and right
		int ofs_x = (physical_width - logical_width * scale_y + 1) / 2;
		x = x * scale_y + ofs_x;
		y = y * scale_y;
	} else {
		// physical aspect ratio is narrower; black borders at top and bottom
		int ofs_y = (physical_height - logical_height * scale_x + 1) / 2;
		x = x * scale_x;
		y = y * scale_x + ofs_y;
	}

	SDL_WarpMouseInWindow(window, x, y);

	return OK;
}

void sdl_mouse_init(void)
{
	nextMouseEvent = 0;
}
