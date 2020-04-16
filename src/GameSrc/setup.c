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
/*
 * $Source: r:/prj/cit/src/RCS/setup.c $
 * $Revision: 1.141 $
 * $Author: dc $
 * $Date: 1994/11/23 00:05:51 $
 */

#define __SETUP_SRC

#include <string.h>

// TODO: extract this into a compatibility header
#ifdef _MSC_VER
	#ifndef F_OK
	#define F_OK 0
	#endif
#else
	#include <unistd.h>
#endif

#include "archiveformat.h"
#include "ShockDialogs.h"
#include "setup.h"
#include "colors.h"
#include "diffq.h"
#include "gamewrap.h"
#include "gr2ss.h"
#include "hud.h"
#include "init.h"
#include "miscqvar.h"
#include "player.h"
#include "version.h"
#include "wrapper.h"
#include "verify.h"
#include "cybstrng.h"
#include "gamestrn.h"

#include <mainloop.h>
#include <tools.h>
#include <input.h>
#include <game_screen.h>
#include <hkeyfunc.h>
#include <loops.h>
#include <keydefs.h>
#include <cybmem.h>
#include <status.h>
#include "cutsloop.h"
#include <wrapper.h>
#include <mlimbs.h>
#include <musicai.h>
#include <palfx.h>
#include <verify.h>
#include <gamescr.h>
#include <gamepal.h>
#include <gamestrn.h>
#include <cybstrng.h>
#include <faketime.h>

#include "2d.h"
#include "splash.h"
#include "splshpal.h"
#include "tickcount.h"
#include "Shock.h"
#include "Xmi.h"

#include <SDL.h>

#ifdef PLAYTEST
#include <mprintf.h>
#endif

#include <mlimbs.h>



#define KEYBOARD_FOCUS_COLOR  (RED_BASE + 3)
#define NORMAL_ENTRY_COLOR    (RED_BASE + 7)
#define CURRENT_DIFF_COLOR    (RED_BASE + 3)
#define SELECTED_COLOR        (RED_BASE)
#define UNAVAILABLE_COLOR     (RED_BASE + 11)



uiSlab setup_slab;
LGRegion setup_root_region;
uchar play_intro_anim;
uchar save_game_exists = FALSE;
SetupMode setup_mode;
SetupMode last_setup_mode;
int intro_num;
int splash_num;
int diff_sum = 0;
bool do_fades = false;
ubyte valid_save;
uchar setup_bio_started = FALSE;
uchar start_first_time = TRUE;
bool waiting_for_key = false;

static uchar direct_into_cutscene = FALSE;

extern char which_lang;
extern uchar clear_player_data;
extern char current_cutscene;
extern char curr_vol_lev;
extern char curr_sfx_vol;
extern uchar fullscrn_vitals;
extern uchar fullscrn_icons;
extern uchar map_notes_on;
extern uchar audiolog_setting;
extern uchar mouseLefty;
#ifdef AUDIOLOGS
extern char curr_alog_vol;
#endif



errtype draw_difficulty_char(int char_num);
errtype draw_difficulty_description(int which_cat, int color);
errtype journey_newgame_func(void);
errtype journey_continue_func(uchar draw_stuff);
errtype draw_username(int color, char *string);

extern errtype load_da_palette();
extern errtype musicai_shutdown(void);
extern int MacTuneLoadTheme(char *theme_base, int themeID);
extern void MacTuneKillCurrentTheme(void);
extern void check_and_update_initial(void);
extern void second_format(int sec_remain, char *s);
extern void pump_events(void);
extern void SDLDraw(void);
#ifdef SVGA_SUPPORT
extern void change_svga_screen_mode();
#endif



//*****************************************************************************

//DIFFICULTY



#define DIFF_DONE_X1            119
#define DIFF_DONE_Y1            179
#define DIFF_DONE_X2            203
#define DIFF_DONE_Y2            198

#define DIFF_NAME_X             57
#define DIFF_NAME_Y             49
#define DIFF_NAME_X2            253
#define DIFF_NAME_Y2            65
#define DIFF_NAME_TEXT_X        124

#define DIFF_X_BASE             28
#define DIFF_W_BASE             156
#define DIFF_W_ELEM             32
#define DIFF_H_ELEM             20

#define DIFF_OPT_TOP            99
#define DIFF_OPT_DELTA          53
#define DIFF_OPT_HEIGHT         24
#define DIFF_STRING_OFFSET_Y    10
#define DIFF_STRING_OFFSET_X    13

#define DIFF_TITLE1_X1          12
#define DIFF_TITLE1_Y1          70
#define DIFF_TITLE1_X2          155
#define DIFF_TITLE1_Y2          93
#define DIFF_TITLE1_OPT_TOP     94
#define DIFF_TITLE1_OPT_BOTTOM  118
#define DIFF_TITLE1_LEFT1       25
#define DIFF_TITLE1_RIGHT1      45

#define DIFF_TITLE2_X1          169
#define DIFF_TITLE2_Y1          70
#define DIFF_TITLE2_X2          311
#define DIFF_TITLE2_Y2          93

#define DIFF_TITLE3_X1          12
#define DIFF_TITLE3_Y1          123
#define DIFF_TITLE3_X2          155
#define DIFF_TITLE3_Y2          146

#define DIFF_TITLE4_X1          169
#define DIFF_TITLE4_Y1          123
#define DIFF_TITLE4_X2          311
#define DIFF_TITLE4_Y2          146

#define DIFF_SIZE_X  DIFF_TITLE1_RIGHT1 - DIFF_TITLE1_LEFT1
#define DIFF_SIZE_Y  DIFF_TITLE1_OPT_BOTTOM - DIFF_TITLE1_OPT_TOP

// why +2?
#define build_diff_x(char_num) \
  ((DIFF_X_BASE + (DIFF_W_ELEM * (char_num & 3)) + (((char_num >> 2) & 1) * DIFF_W_BASE)) + 2)
#define build_diff_y(char_num) \
  ((DIFF_OPT_TOP + ((char_num >> 3) * DIFF_OPT_DELTA)) + 2)

#define NUM_DIFF_CATEGORIES  4

#define CATEGORY_STRING_BASE  REF_STR_diffCategories
#define DIFF_STRING_BASE      REF_STR_diffStrings
#define DIFF_NAME             REF_STR_diffName
#define DIFF_START            REF_STR_diffStart

#define FLASH_TIME  (CIT_CYCLE / 8)

#define COMPUTE_DIFF_STRING_X(wcat)  (DIFF_X_BASE + (DIFF_W_BASE * (wcat & 1)) - DIFF_STRING_OFFSET_X)
#define COMPUTE_DIFF_STRING_Y(wcat)  (DIFF_OPT_TOP + ((wcat >> 1) * DIFF_OPT_DELTA) - DIFF_STRING_OFFSET_Y)

#define REF_IMG_bmDifficultyScreen  0x26d0000



char curr_diff = 0;
uchar start_selected = FALSE;

short diff_titles_x[] = {DIFF_TITLE1_X1, DIFF_TITLE2_X1, DIFF_TITLE3_X1, DIFF_TITLE4_X1};
short diff_titles_y[] = {DIFF_TITLE1_Y1, DIFF_TITLE2_Y1, DIFF_TITLE3_Y1, DIFF_TITLE4_Y1};
char *valid_char_string = "0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz ";

LGRect name_rect = {{DIFF_NAME_TEXT_X, DIFF_NAME_Y}, {DIFF_NAME_X2, DIFF_NAME_Y2}};



errtype compute_new_diff(void)
{
  int i, new_sum = 0;

  for (i = 0; i < 4; i++)
    new_sum += player_struct.difficulty[i];
  diff_sum = new_sum;

  return OK;
}



errtype difficulty_draw(uchar full)
{
  int i;

  uiHideMouse(NULL);

  if (full)
  {
    draw_raw_res_bm_temp(REF_IMG_bmDifficultyScreen, 0, 0);
    status_bio_draw();
  }

  setup_mode = SETUP_DIFFICULTY;

  for (i = 0; i < NUM_DIFF_CATEGORIES; i++)
  {
    if (i == curr_diff && !start_selected) gr_set_fcolor(KEYBOARD_FOCUS_COLOR);
    else                                   gr_set_fcolor(NORMAL_ENTRY_COLOR);

    res_draw_string(RES_citadelFont, CATEGORY_STRING_BASE + i, diff_titles_x[i] + 12, diff_titles_y[i] + 2);
  }

  if (start_selected) gr_set_fcolor(KEYBOARD_FOCUS_COLOR);
  else                gr_set_fcolor(NORMAL_ENTRY_COLOR);

  res_draw_string(RES_citadelFont, DIFF_START, DIFF_DONE_X1 + 13, DIFF_DONE_Y1 + 2);

  if (full)
  {
    for (i = 0; i < 16; i++) draw_difficulty_char(i);
    for (i = 0; i < 4; i++) draw_difficulty_description(i, NORMAL_ENTRY_COLOR);

    gr_set_fcolor(KEYBOARD_FOCUS_COLOR);
    res_draw_string(RES_citadelFont, DIFF_NAME, DIFF_NAME_X, DIFF_NAME_Y);
    draw_username(NORMAL_ENTRY_COLOR, player_struct.name);
  }

  uiShowMouse(NULL);

  return OK;
}



errtype draw_username(int color, char *string)
{
  gr_set_fcolor(color);
  uiHideMouse(&name_rect);
  res_draw_text(RES_citadelFont, string, DIFF_NAME_TEXT_X, DIFF_NAME_Y);
  uiShowMouse(&name_rect);

  return OK;
}



void flash_username(void)
{
  long flash_done;

  uiHideMouse(&name_rect);
  gr_set_fcolor(SELECTED_COLOR - 4);
  res_draw_string(RES_citadelFont, DIFF_NAME, DIFF_NAME_X, DIFF_NAME_Y);
  flash_done = *tmd_ticks + FLASH_TIME;
  while (TickCount() < flash_done)
  {
    SDLDraw();
  }
  gr_set_fcolor(KEYBOARD_FOCUS_COLOR);
  res_draw_string(RES_citadelFont, DIFF_NAME, DIFF_NAME_X, DIFF_NAME_Y);
  uiShowMouse(&name_rect);
}



errtype draw_difficulty_line(int which_line)
{
  int i;

  for (i = 0; i < 4; i++) draw_difficulty_char((which_line * 4) + i);
  draw_difficulty_description(which_line, NORMAL_ENTRY_COLOR);

  return OK;
}



errtype draw_difficulty_description(int which_cat, int color)
{
  if (color != -1) gr_set_fcolor(color);
  res_draw_string(RES_smallTechFont, DIFF_STRING_BASE + (which_cat * 4) + player_struct.difficulty[which_cat],
                  COMPUTE_DIFF_STRING_X(which_cat), COMPUTE_DIFF_STRING_Y(which_cat));

  return OK;
}



// char_num 1-16
errtype draw_difficulty_char(int char_num)
{
  char buff[] = "X";

  uiHideMouse(NULL);
  if (player_struct.difficulty[char_num / 4] == char_num % 4) gr_set_fcolor(CURRENT_DIFF_COLOR);
  else                                                        gr_set_fcolor(NORMAL_ENTRY_COLOR);
  buff[0] = (char_num & 3) + '0';
  res_draw_text(RES_citadelFont, buff, build_diff_x(char_num), build_diff_y(char_num));
  uiShowMouse(NULL);

  return OK;
}
//*****************************************************************************



//*****************************************************************************

//JOURNEY



#define JOURNEY_OPT1_TOP  66
#define JOURNEY_OPT1_BOT  88

#define JOURNEY_OPT2_TOP  97
#define JOURNEY_OPT2_BOT  119

#define JOURNEY_OPT3_TOP  129
#define JOURNEY_OPT3_BOT  160

#define JOURNEY_OPT4_TOP  160
#define JOURNEY_OPT4_BOT  182

#define SETUP_STRING_BASE  REF_STR_journeyOpts

#define NUM_SETUP_LINES  4

#define JOURNEY_OPT_LEFT   79
#define JOURNEY_OPT_RIGHT  247

#define REF_IMG_bmJourneyOnwards  0x26c0000



#ifdef DEMO
char curr_setup_line = 1;
#else
char curr_setup_line = 0;
#endif

int journey_y[8] = {JOURNEY_OPT1_TOP, JOURNEY_OPT1_BOT, JOURNEY_OPT2_TOP, JOURNEY_OPT2_BOT,
                    JOURNEY_OPT3_TOP, JOURNEY_OPT3_BOT, JOURNEY_OPT4_TOP, JOURNEY_OPT4_BOT};

short setup_tops[] = {JOURNEY_OPT1_TOP, JOURNEY_OPT2_TOP, JOURNEY_OPT3_TOP, JOURNEY_OPT4_TOP};



errtype journey_draw(char part)
{
  char i;

  uiHideMouse(NULL);

  if (setup_bio_started)
  {
    status_bio_end();
    setup_bio_started = FALSE;
  }

  // extract into buffer - AFTER we've stopped biorhythms (which used that buffer.....)
  if (part == 0) draw_raw_res_bm_temp(REF_IMG_bmJourneyOnwards, 0, 0);

  for (i = 0; i < NUM_SETUP_LINES; i++)
  {
    if ((part == 0) || (part - 1 == i))
    {
      int col;

      if (i == curr_setup_line) col = KEYBOARD_FOCUS_COLOR;
      else                      col = NORMAL_ENTRY_COLOR;

#ifdef DEMO
      if ((i == NUM_SETUP_LINES - 1) || (i == 0))
#else
      if (i == NUM_SETUP_LINES - 1) // why is NUM_SETUP_LINES-1 necessarily continue?
#endif
      {

#ifndef DEMO
        if (!save_game_exists)
#endif

          col = UNAVAILABLE_COLOR;
      }
      gr_set_fcolor(col);
      res_draw_string(RES_citadelFont, SETUP_STRING_BASE + i, JOURNEY_OPT_LEFT + 15, setup_tops[i] + 4);
    }
  }
  uiShowMouse(NULL);

  setup_mode = SETUP_JOURNEY;

  return OK;
}



errtype journey_intro_func(uchar draw_stuff)
{

#ifdef DEMO
  uiShowMouse(NULL); // need to leave it hidden
  return OK;
#else
  if (draw_stuff)
    res_draw_string(RES_citadelFont, SETUP_STRING_BASE, JOURNEY_OPT_LEFT + 15, JOURNEY_OPT1_TOP + 2);
  uiShowMouse(NULL); // need to leave it hidden

  MacTuneKillCurrentTheme();

  return play_cutscene(START_CUTSCENE, FALSE);
#endif

}



errtype journey_newgame_func(void)
{
  clear_player_data = TRUE;

  DEBUG("Load object data");
  object_data_load();

  player_struct.level = 0xFF;

  DEBUG("Create initial game");
  create_initial_game_func(0, 0, 0);

  INFO("Started new game!");

  change_mode_func(0, 0, GAME_LOOP);

  return OK;
}



errtype journey_difficulty_func(uchar draw_stuff)
{
  if (draw_stuff)
    res_draw_string(RES_citadelFont, SETUP_STRING_BASE + 1, JOURNEY_OPT_LEFT + 15, JOURNEY_OPT2_TOP + 2);
  uiShowMouse(NULL);
  difficulty_draw(TRUE);
  compute_new_diff();
  status_bio_set(DIFF_BIO);
  status_bio_start();
  setup_bio_started = TRUE;

  return OK;
}
//*****************************************************************************



//*****************************************************************************

//CREDITS



#define CredResFnt    (RES_coloraliasedFont)
#define CredColor     (GREEN_BASE + 4)
#define CredResource  (RES_credits)



int credits_inp = 0;

void *credits_txtscrn;

int CreditsTune;

//set this when game is won, then stats will be shown once before credits
int WonGame_ShowStats = 0;



//ticks: 0 wait forever
int WaitForKey(ulong ticks)
{
  ulong end_ticks = (ulong)TickCount();
  ulong key_ticks = end_ticks + (!ticks ? 500 : (ticks*1/8));
  end_ticks = ticks ? end_ticks + ticks : 0;
  int ch;

  //wait for specified elapsed ticks or keypress
  for (;;)
  {


    //loop win or elevator music
    int i = 0;
    if (music_on && !IsPlaying(i))
    {
      int track;

      if (WonGame_ShowStats) track = 0;
      else                   track = 1+CreditsTune;

      if (track >= 0 && track < NumTracks)
      {
//        int volume = (int)curr_vol_lev * 127 / 100; //convert from 0-100 to 0-127
        StartTrack(i, track);

        if (!WonGame_ShowStats) CreditsTune = (CreditsTune + 1) % 8;
      }
    }


    pump_events();
    SDLDraw();

    kbs_event ev = kb_next();
    ch = ev.ascii;
    ticks = (ulong)TickCount();

    if ((ch == 27 || ch == ' ' || ch == '\r') && ticks >= key_ticks) break;
    if (end_ticks && ticks >= end_ticks) break;
  }

  return ch;
}



void PrintWinStats(void)
{
  char buf[256], buf_temp[256];
  int x, y = 15;
  short w, h;

  grs_font *fon = gr_get_font();
  gr_set_font(ResLock(RES_coloraliasedFont));

  gr_clear(0);

  sprintf(buf, "CONGRATULATIONS!");
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12 * 2;

  sprintf(buf, "YOU HAVE COMPLETED SYSTEM SHOCK!");
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12;

  sprintf(buf, "HIT ESC TO VIEW CREDITS.");
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12 * 2;

  sprintf(buf, "STATISTICS");
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12;

  //underline
  short x1 = SCONV_X((320-w)/2);
  short x2 = SCONV_X((320-w)/2+w-1);
  for (; x1 <= x2; x1++)
  {
    short y1 = SCONV_Y(y);
    short y2 = SCONV_Y(y+1);
    y2 = y1 + (y2-y1) / 3;
    for (; y1 <= y2; y1++) gr_set_pixel(GREEN_BASE + 4, x1, y1);
  }

  y += 4;

  sprintf(buf, "TIME: %u", player_struct.game_time);
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12;

  sprintf(buf, "KILLS: %d", player_struct.num_victories);
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12;

  sprintf(buf, "REGENERATIONS: %d", player_struct.num_deaths);
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12;

  uint8_t stupid = 0;
  for (uint8_t i = 0; i < 4; i++)
    stupid += (player_struct.difficulty[i] * player_struct.difficulty[i]);
  sprintf(buf, "DIFFICULTY INDEX: %d", stupid);
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y); y += 12;

  int score;
  // death is 10 anti-kills, but you always keep at least a third of your kills.
  score = player_struct.num_victories -
            lg_min(player_struct.num_deaths * 10, player_struct.num_victories * 2 / 3);
  score = score * 10000;
  score = score - lg_min(score * 2 / 3, ((player_struct.game_time / (CIT_CYCLE * 36)) * 100));
  score = score * (stupid + 1) / 37; // 9 * 4 + 1 is best difficulty factor
  if (stupid == 36) {
      score += 2222222; // secret kevin bonus
  }
  sprintf(buf, "SCORE: %d", score);
  gr_string_size(buf, &w, &h); ss_string(buf, (320-w)/2, y);

  ResUnlock(RES_coloraliasedFont);
  gr_set_font(fon);

  WaitForKey(0);
}



void PrintCredits(void)
{
  //Credits display reverse-engineered from text resources

  int end = 0, line = 0, x, y = 15, columns = 1, cur_col = 0;
  int underline = 0, last_underline = 0;
  char buf[256];

  gr_clear(0);

  while (!end)
  {
    get_string((RES_credits << 16) | line, buf, sizeof(buf));
    line++;

    int len = strlen(buf);

    if (*buf == '^')
    {
      for (int i=1; i<len; i++)
      {
        switch (buf[i])
        {
          case 'E': WaitForKey(0); end = 1; break;
          case 'p': WaitForKey(200); break;
          case 'G': if (WaitForKey(2000) == 27) end = 1; gr_clear(0); y = 15; break;
          case 'N': break; //dunno
          case '1': columns = 1; cur_col = 0; break;
          case '2': columns = 2; cur_col = 0; break;
          case 'H': underline = 3; break;
          case 'T': underline = 2; break;
          case 'h': underline = 1; break;
          case 'c': underline = 0; break;
          case 'S': y += 10; break;
          case 'L': y = 15 * (buf[++i] - '0'); break;
        }
      }
      continue;
    }

    grs_font *fon = gr_get_font();
    gr_set_font(ResLock(RES_coloraliasedFont));
    short w, h;
    gr_string_size(buf, &w, &h);
    x = (columns == 1) ? (320-w)/2 : (cur_col == 0) ? 320/2-8-w : 320/2+8;
    ss_string(buf, x, y);
    ResUnlock(RES_coloraliasedFont);
    gr_set_font(fon);

    if (underline)
    {
      short x1, x2, y1, y2;

      x1 = SCONV_X(x);
      x2 = SCONV_X(x+w-1);
      for (; x1 <= x2; x1++)
      {
        y1 = SCONV_Y(y+h+1);
        y2 = SCONV_Y(y+h+2);
        if (underline == 2) y2 = y1 + (y2-y1) / 2;
        else y2 = y1 + (y2-y1) / 3;
        for (; y1 <= y2; y1++) gr_set_pixel(GREEN_BASE + 4, x1, y1);
      }

      if (underline == 3)
      {
        x1 = SCONV_X(x + 1);
        x2 = SCONV_X(x+w-1 - 1);
        for (; x1 <= x2; x1++)
        {
          y1 = SCONV_Y(y+h+1);
          y2 = SCONV_Y(y+h+2);
          y1 = y1 + (y2-y1) / 3;
          for (; y1 <= y2; y1++) gr_set_pixel(GREEN_BASE + 4, x1, y1);
        }
      }
    }

    if (columns == 1) {y += underline ? 14 : 11; underline = 0;}
    else
    {
      if (!cur_col) last_underline = underline;
      else {y += (underline || last_underline) ? 14 : 11; underline = 0;}
      cur_col ^= 1;
    }
  }
}



errtype journey_credits_func(uchar draw_stuff)
{
  setup_mode = SETUP_CREDITS;

  if (draw_stuff)
  {
    if (music_on)
    {
      if (WonGame_ShowStats)
      {
        MacTuneLoadTheme("endloop", 0);
      }
      else
      {
        CreditsTune = 0;
        load_score_guts(7); //elevator
      }
    }

    if (WonGame_ShowStats) PrintWinStats();
    PrintCredits();

    WonGame_ShowStats = 0;

    journey_credits_done();
  }
  else
  {
    if (credits_inp != 0)
    {
      journey_credits_done();
    }
  }

  return OK;
}



void journey_credits_done(void)
{
  kb_flush();
  mouse_flush();

  credits_inp = 0;

  uiShowMouse(NULL);

  journey_draw(0);

  if (music_on) MacTuneLoadTheme("titloop", 0);
}
//*****************************************************************************



//*****************************************************************************

//CONTINUE



#define SG_SLOT_HT        17
#define SG_SLOT_WD        169
#define SG_SLOT_OFFSET_X  21
#define SG_SLOT_OFFSET_Y  6
#define SG_SLOT_X         79
#define SG_SLOT_Y         62

#define REF_IMG_bmContinueScreen  0x26e0000



char curr_sg = 0;



errtype draw_sg_slot(int slot_num)
{
  char temp[64];
  short sz, x, y;

  uiHideMouse(NULL);

  if (curr_sg == slot_num) gr_set_fcolor(KEYBOARD_FOCUS_COLOR);
  else                     gr_set_fcolor(NORMAL_ENTRY_COLOR);

  // if slot_num == -1 highlight the curr_sg slot with the SELECTED_COLOR color
  if (slot_num == -1)
  {
    gr_set_fcolor(SELECTED_COLOR);
    slot_num = curr_sg;
  }

  if (valid_save & (1 << slot_num))
  {
    sz = strlen(comments[slot_num]);
    strcpy(temp, comments[slot_num]);
  }
  else
  {
    get_string(REF_STR_UnusedSave, temp, 64);
  }

  gr_set_font(ResLock(RES_smallTechFont));
  gr_string_size(temp, &x, &y);

  while ((x > SG_SLOT_WD - SG_SLOT_OFFSET_X) && (sz > 0))
  {
    sz--;
    strcpy(temp, "");
    strncpy(temp, comments[slot_num], sz);
    temp[sz] = '\0';
    gr_string_size(temp, &x, &y);
  }

  ResUnlock(RES_smallTechFont); // was RES_CitadelFont

  res_draw_text(RES_smallTechFont, temp, SG_SLOT_X + SG_SLOT_OFFSET_X,
                SG_SLOT_Y + SG_SLOT_OFFSET_Y + (slot_num * SG_SLOT_HT));

  uiShowMouse(NULL);

  return OK;
}



errtype draw_savegame_names(void)
{
  int i;

  for (i = 0; i < NUM_SAVE_SLOTS; i++) draw_sg_slot(i);

  return OK;
}



errtype load_that_thar_game(int which_slot)
{
  DEBUG("load_that_thar_game %i", which_slot);

  errtype retval;

  if (valid_save & (1 << which_slot))
  {
    draw_sg_slot(-1); // highlight the current save game slot with SELECTED_COLOR

    clear_player_data = TRUE; // initializes the player struct in object_data_load
    object_data_load();
    player_create_initial();
    player_struct.level = 0xFF; // make sure we load textures
    Poke_SaveName(which_slot);
    change_mode_func(0, 0, GAME_LOOP);
    retval = load_game(save_game_name);
    if (retval != OK)
    {
      strcpy(comments[which_slot], "<< INVALID GAME >>");
      uiHideMouse(NULL);
      journey_continue_func(TRUE);
      return retval;
    }

    gr2ss_override = OVERRIDE_ALL; // CC: This fixed popups cursors drawing tiny after loading
  }

  return OK;
}



errtype journey_continue_func(uchar draw_stuff)
{

#ifndef DEMO
  if (save_game_exists)
  {
    //draw_raw_res_bm_extract(REF_IMG_bmContinueScreen, 0, 0);

    //do what the above line does, but hack bitmap height
    Ref rid = REF_IMG_bmContinueScreen;
    int i = REFINDEX(rid);
    RefTable *rt = ResReadRefTable(REFID(rid));
    if (RefIndexValid(rt, i))
    {
	FrameDesc *f = RefLock(rid);
	grs_bitmap bm = f->bm;
        bm.h = 200; //SUPER HACK: resource reports 320
        ss_bitmap(&bm, 0, 0);
        RefUnlock(rid);
    }
    ResFreeRefTable(rt);

    setup_mode = SETUP_CONTINUE;
    draw_savegame_names();
  }
#endif

  uiShowMouse(NULL);

  return OK;
}
//*****************************************************************************



//*****************************************************************************

//SETUP



#define DO_FADES

#define SECRET_MISSION_DIFFICULTY_QB  0xB0

#define ALT(x) ((x) | KB_FLAG_ALT)

#define CFG_NAME_VAR  "name"



char diff_qvars[4] = {COMBAT_DIFF_QVAR, MISSION_DIFF_QVAR, PUZZLE_DIFF_QVAR, CYBER_DIFF_QVAR};

typedef errtype (*journey_func)(uchar draw_stuff);

journey_func journey_funcs[4] = {journey_intro_func, journey_difficulty_func, journey_credits_func,
                                 journey_continue_func};

// if there are two different input events - only lets one call a journey_func
uchar journey_lock = FALSE;



void go_and_start_the_game_already(void)
{
  INFO("New Journey");

  char i;

#ifdef GAMEONLY
  if (strlen(player_struct.name) == 0)
  {
    flash_username();
    return;
  }
#endif

  uiHideMouse(NULL);
  gr_set_fcolor(SELECTED_COLOR);
  res_draw_string(RES_citadelFont, DIFF_START, DIFF_DONE_X1 + 13, DIFF_DONE_Y1 + 2);
  uiShowMouse(NULL);

  journey_newgame_func();

#ifdef SVGA_SUPPORT
  QUESTVAR_SET(SCREENMODE_QVAR, convert_use_mode);
#endif

  QUESTVAR_SET(MUSIC_VOLUME_QVAR, (curr_vol_lev * curr_vol_lev) / 100);
  QUESTVAR_SET(SFX_VOLUME_QVAR, (curr_sfx_vol * curr_sfx_vol) / 100);

#ifdef AUDIOLOGS
  QUESTVAR_SET(ALOG_VOLUME_QVAR, (curr_alog_vol * curr_alog_vol) / 100);
  QUESTVAR_SET(ALOG_OPT_QVAR, audiolog_setting);
#endif

  QUESTVAR_SET(FULLSCRN_ICON_QVAR, fullscrn_icons);
  QUESTVAR_SET(FULLSCRN_VITAL_QVAR, fullscrn_vitals);
  QUESTVAR_SET(AMAP_NOTES_QVAR, map_notes_on);
  QUESTVAR_SET(HUDCOLOR_QVAR, hud_color_bank);
  QUESTVAR_SET(SCREENMODE_QVAR, 3);
  QUESTVAR_SET(DCLICK_QVAR, FIX_UNIT / 3);

  for (i = 0; i < 4; i++) QUESTVAR_SET(diff_qvars[i], player_struct.difficulty[i]);
  if (QUESTVAR_GET(MISSION_DIFF_QVAR) == 3) hud_set(HUD_GAMETIME);

  strncpy(player_struct.version, SYSTEM_SHOCK_VERSION, 6);

  if (setup_bio_started)
  {
    status_bio_end();
    setup_bio_started = FALSE;
  }

  gr2ss_override = OVERRIDE_ALL; // CC: This fixed popups cursors drawing tiny
}



uchar intro_mouse_handler(uiEvent *ev, LGRegion *r, intptr_t user_data)
{
  int which_one = -1;
  int i = 0;
  int old_diff;
  uchar diff_changed;

#ifndef NO_DUMMIES
  intptr_t dummy = user_data;
  LGRegion *dummy2 = r;
#endif

  if (ev->mouse_data.action & MOUSE_LDOWN)
  {
    // If in the splash screen, advance
    if (waiting_for_key)
    {
      waiting_for_key = false;
      return OK;
    }

    switch (setup_mode)
    {
      case SETUP_JOURNEY:
        if (!journey_lock)
        {
          if ((ev->pos.x > JOURNEY_OPT_LEFT) && (ev->pos.x < JOURNEY_OPT_RIGHT))
          {
            while ((which_one == -1) && (i <= 6))
            {
              if ((ev->pos.y > journey_y[i]) && (ev->pos.y < journey_y[i + 1])) which_one = i >> 1;
              else i += 2;
            }
            TRACE("%s: which_one = %d", __FUNCTION__, which_one);
            if (which_one != -1)
            {
              uiHideMouse(NULL);
              gr_set_fcolor(SELECTED_COLOR);
              journey_lock = TRUE;
              journey_funcs[which_one](TRUE);
              journey_lock = FALSE;
            }
          }
        }
      break;

      case SETUP_CREDITS:
        credits_inp = -1;
      break;

      case SETUP_CONTINUE:
        if ((ev->pos.x >= SG_SLOT_X) && (ev->pos.x <= SG_SLOT_X + SG_SLOT_WD) && (ev->pos.y >= SG_SLOT_Y) &&
            (ev->pos.y <= SG_SLOT_Y + (NUM_SAVE_SLOTS * SG_SLOT_HT)))
        {
          char which = (ev->pos.y - SG_SLOT_Y) / SG_SLOT_HT;
          char old_sg = curr_sg;

          curr_sg = which;
          draw_sg_slot(old_sg);
          load_that_thar_game(which);
        }
      break;

      case SETUP_DIFFICULTY:
        diff_changed = FALSE;
        // given that these are all rectangles, i bet you could just get mouse pos and divide, eh?
        for (i = 0; i < 16; i++)
        {
          if ((ev->pos.x > (build_diff_x(i) - 2)) && (ev->pos.x < (build_diff_x(i) - 2 + DIFF_SIZE_X)) &&
              (ev->pos.y > (build_diff_y(i) - 2)) && (ev->pos.y < (build_diff_y(i) - 2 + DIFF_SIZE_Y)))
          {
            old_diff = player_struct.difficulty[i / 4];
            draw_difficulty_description(i / 4, 0);
            player_struct.difficulty[i / 4] = i % 4;
            TRACE("%s: difficulty %d set to %d (verify = %d)", __FUNCTION__, i / 4, i % 4, player_struct.difficulty[i / 4]);
            draw_difficulty_char(((i / 4) * 4) + old_diff);
            draw_difficulty_char(i);
            draw_difficulty_description(i / 4, NORMAL_ENTRY_COLOR);
            diff_changed = TRUE;
          }
        }
        if (diff_changed) compute_new_diff();
        else if ((ev->pos.x > DIFF_DONE_X1) && (ev->pos.x < DIFF_DONE_X2) && (ev->pos.y > DIFF_DONE_Y1) &&
                 (ev->pos.y < DIFF_DONE_Y2))
          go_and_start_the_game_already();
      break;
    }
  }

  return TRUE;
}



uchar intro_key_handler(uiEvent *ev, LGRegion *r, intptr_t user_data)
{
  int code = ev->cooked_key_data.code & ~(KB_FLAG_DOWN | KB_FLAG_2ND);
  char old_diff, old_setup_line = curr_setup_line, n = 0;

  if (ev->cooked_key_data.code & KB_FLAG_DOWN)
  {
    // If in the splash screen, advance
    if (waiting_for_key)
    {
      waiting_for_key = false;
      return OK;
    }

    switch (setup_mode)
    {
      case SETUP_JOURNEY:
        switch (code)
        {
          case KEY_UP:
            n = NUM_SETUP_LINES - 2; // sneaky fallthrough action
          case KEY_DOWN:
            n++;
            curr_setup_line = (curr_setup_line + n) % NUM_SETUP_LINES;
#ifdef DEMO
            if (curr_setup_line == NUM_SETUP_LINES - 1) // why is NUM_SETUP_LINES-1 necessarily continue?
              curr_setup_line = 2;
            if (curr_setup_line == 0)
              curr_setup_line = 1;
#else
            if (curr_setup_line == NUM_SETUP_LINES - 1) // why is NUM_SETUP_LINES-1 necessarily continue?
              if (!save_game_exists)
                curr_setup_line = (curr_setup_line + n) % NUM_SETUP_LINES;
#endif
            journey_draw(old_setup_line + 1);
            journey_draw(curr_setup_line + 1);
          break;

          case KEY_ENTER:
            if (!journey_lock)
            {
              uiHideMouse(NULL);
              gr_set_fcolor(SELECTED_COLOR);
              journey_lock = TRUE;
              journey_funcs[curr_setup_line](TRUE);
              journey_lock = FALSE;
            }
          break;
        }
      break;

      case SETUP_CREDITS:
        credits_inp = code;
      break;

      case SETUP_CONTINUE:
        switch (code)
        {
          case KEY_UP:
          case KEY_LEFT:
            n = NUM_SAVE_SLOTS - 2;
          case KEY_DOWN:
          case KEY_RIGHT:
            n++;
            old_diff = curr_sg;
            curr_sg = (curr_sg + n) % NUM_SAVE_SLOTS;
            draw_sg_slot(old_diff);
            draw_sg_slot(curr_sg);
          break;

          case KEY_ENTER:
            load_that_thar_game(curr_sg);
          break;

          case KEY_ESC:
            journey_draw(0);
          break;
        }
      break;

      case SETUP_DIFFICULTY:
        switch (code)
        {
          case ALT('X'): // Don't print the X when user ALT-X's out of the game
          case ALT('x'):
          break;

          case '-':
          case KEY_LEFT:
            n = NUM_DIFF_CATEGORIES - 2; // note sneaky -2 for fallthrough
          case '+':
          case KEY_RIGHT:
            n++; // n now NDC-1 or 1
            if (!start_selected)
            {
              old_diff = player_struct.difficulty[curr_diff];
              draw_difficulty_description(curr_diff, 0);
              player_struct.difficulty[curr_diff] =
                (player_struct.difficulty[curr_diff] + n) % NUM_DIFF_CATEGORIES;
              draw_difficulty_char(curr_diff * NUM_DIFF_CATEGORIES + player_struct.difficulty[curr_diff]);
              draw_difficulty_char(curr_diff * NUM_DIFF_CATEGORIES + old_diff);
              draw_difficulty_description(curr_diff, NORMAL_ENTRY_COLOR);
              compute_new_diff();
            }
          break;

          case KEY_UP:
          case (KEY_TAB | KB_FLAG_SHIFT):
            n = NUM_DIFF_CATEGORIES - 2; // sneaky fallthrough
          case KEY_DOWN:
          case KEY_TAB:
            n++; // now -1 or 1
            if (start_selected && n == 1)
            {
              start_selected = FALSE;
              curr_diff = 0;
            }
            else if (start_selected && n == NUM_DIFF_CATEGORIES - 1)
            {
              start_selected = FALSE;
              curr_diff = NUM_DIFF_CATEGORIES - 1;
            }
            else if ((curr_diff == NUM_DIFF_CATEGORIES - 1 && n == 1) ||
                     (curr_diff == 0 && n == NUM_DIFF_CATEGORIES - 1))
              start_selected = TRUE;
            else
            {
              start_selected = FALSE;
              curr_diff = (curr_diff + n) % NUM_DIFF_CATEGORIES;
            }
            difficulty_draw(FALSE);
          break;

          case KEY_ENTER:
            go_and_start_the_game_already();
          break;

          case KEY_ESC:
            journey_draw(0);
          break;

          default:
	  {
            draw_username(0, player_struct.name);
            n = strlen(player_struct.name);
	    short c = ev->cooked_key_data.code;
            if (kb_isprint(c) &&
                (n < sizeof(player_struct.name)) &&
                (((c & 0xff) >= 128 && (c & 0xff) <= 155) ||
                 ((c & 0xff) >= 160 && (c & 0xff) <= 165) ||
                 strchr(valid_char_string, c & 0xFF) != NULL))
            {
              player_struct.name[n] = (c & 0xFF);
              player_struct.name[n + 1] = '\0';
            }
            if (((c & 0xFF) == KEY_BS) && (n > 0))
              player_struct.name[n - 1] = '\0';
            draw_username(NORMAL_ENTRY_COLOR, player_struct.name);
	  }
          break;
        }
      break;
    }
  }

  return (main_kb_callback(ev, r, user_data));
}



errtype load_savegame_names(void)
{
  int i;
  int file;

  valid_save = 0;

  DEBUG("Grabbing save game names");

  for (i = 0; i < NUM_SAVE_SLOTS; i++)
  {
    Poke_SaveName(i);

    if (access(save_game_name, F_OK) != -1)
    {
      file = ResOpenFile(save_game_name);
      if (ResInUse(OLD_SAVE_GAME_ID_BASE))
      {
#ifdef OLD_SG_FORMAT
	  ResExtract(OLD_SAVE_GAME_ID_BASE, FORMAT_RAW, comments[i]);
                valid_save |= (1 << i);
#else
                strcpy(comments[i], "<< BAD VERSION >>");
#endif
      }
      else
      {
        if (ResInUse(SAVELOAD_VERIFICATION_ID))
        {
          int verify_cookie;

          ResExtract(SAVELOAD_VERIFICATION_ID, FORMAT_U32, &verify_cookie);
          switch (verify_cookie)
          {
            case OLD_VERIFY_COOKIE_VALID:
              // Uncomment these lines to reject Shock Floppy save games
              //                     sprintf(comments[i], "<< %s >>",get_temp_string(REF_STR_BadVersion + 1));
              //                     break;

            case VERIFY_COOKIE_VALID:
		ResExtract(SAVE_GAME_ID_BASE, FORMAT_RAW, comments[i]);
              valid_save |= (1 << i);
            break;

            default:
              sprintf(comments[i], "<< %s >>", get_temp_string(REF_STR_BadVersion));
            break;
          }
        }
        else sprintf(comments[i], "<< %s >>", get_temp_string(REF_STR_BadVersion));
      }

      ResCloseFile(file);
    }
    else *(comments[i]) = '\0';
  }

  return OK;
}



errtype setup_init(void)
{
#ifndef GAMEONLY
  int data[1];
  int cnt;
#endif

  generic_reg_init(TRUE, &setup_root_region, NULL, &setup_slab, intro_key_handler, intro_mouse_handler);

#ifndef GAMEONLY
  cnt = 1;
  // if (config_get_value("intro", CONFIG_INT_TYPE, data, &cnt))
  {
    physics_running = TRUE;
    time_passes = TRUE;
    _current_loop = SETUP_LOOP;
  }
  if (!config_get_raw(CFG_NAME_VAR, player_struct.name, 40))
    strcpy(player_struct.name, get_temp_string(REF_STR_DefaultPlayName));
  load_savegame_names();
#endif

  setup_mode = SETUP_JOURNEY;

  return OK;
}



void pause_for_key(ulong wait_time)
{
  waiting_for_key = true;

  ulong wait_until = TickCount() + wait_time;

  while (waiting_for_key && ((ulong)TickCount() < wait_until))
  {
    input_chk();
    pump_events();
    SDLDraw();
  }

  waiting_for_key = false;
}



void splash_draw(bool show_splash)
{
  int pal_file;

  if (!show_splash) return;

  // Need to load the splash palette file

  INFO("Loading splshpal.res");
  pal_file = ResOpenFile("res/data/splshpal.res");

  INFO("Loading splash.res");
  splash_num = ResOpenFile("res/data/splash.res");

  if (pal_file < 0) INFO("Could not open splshpal.res!");

  uchar splash_pal[768];
  ResExtract(RES_splashPalette, FORMAT_RAW, splash_pal);

  // Set initial palette

  gr_set_pal(0, 256, splash_pal);

  // Set screen mode

#ifdef SVGA_SUPPORT
  change_svga_screen_mode();
#endif

  // clear the screen
  gr_clear(0);

  HotkeyContext = SETUP_CONTEXT;
  uiSetCurrentSlab(&setup_slab);

  // Draw Origin Logo

#ifdef DO_FADES
  do_fades = true && pal_fx_on;
#endif

  uiHideMouse(NULL);
  draw_full_res_bm(REF_IMG_bmOriginSplash, 0, 0, do_fades);
  pause_for_key(500);

  if (do_fades) palfx_fade_down();

  // Draw LGS Logo

  uiHideMouse(NULL);
  draw_full_res_bm(REF_IMG_bmLGSplash, 0, 0, do_fades);
  pause_for_key(500);

  if (do_fades) palfx_fade_down();

  // Draw System Shock title

  uiHideMouse(NULL);
  draw_full_res_bm(REF_IMG_bmSystemShockTitle, 0, 0, do_fades);
  pause_for_key(500);

  if (do_fades) palfx_fade_down();

  // Original palette
  gr_set_pal(0, 256, ppall);
}



void setup_loop(void)
{
  bool draw_stuff = FALSE;


  //loop title music
  int i = 0;
  if (music_on && !IsPlaying(i))
  {
    int track = 0;
    if (track >= 0 && track < NumTracks)
    {
//      int volume = (int)curr_vol_lev * 127 / 100; //convert from 0-100 to 0-127
      StartTrack(i, track);
    }
  }


  if (last_setup_mode != setup_mode)
  {
    uiHideMouse(NULL);
    gr_clear(0xFF);
    draw_stuff = TRUE;
  }

  last_setup_mode = setup_mode;

  switch (setup_mode)
  {
    case SETUP_DIFFICULTY: difficulty_draw(draw_stuff); break;
    case SETUP_JOURNEY:    journey_draw(draw_stuff); break;
    case SETUP_CONTINUE:   journey_continue_func(draw_stuff); break;
    case SETUP_CREDITS:    journey_credits_func(draw_stuff); break;
  }
}

//if these don't get reset, sticky residue of any old game sticks around
void empty_slate(void)
{
  extern int flush_resource_cache(void);
  flush_resource_cache();

  extern uint _fr_glob_flags;
  _fr_glob_flags = 0;

  extern uchar *shodan_bitmask;
  shodan_bitmask = NULL;

  extern uchar alternate_death;
  alternate_death = FALSE;

  extern uiSlab fullscreen_slab;
  extern uiSlab main_slab;
  uiPopSlabCursor(&fullscreen_slab);
  uiPopSlabCursor(&main_slab);

  extern uchar fire_slam;
  fire_slam = FALSE;

  extern short inventory_page;
  inventory_page = 0;

  extern short inv_last_page;
  inv_last_page = -1;

  extern void physics_zero_all_controls(void);
  physics_zero_all_controls();

  extern int mlook_enabled;
  mlook_enabled = 0;

  extern uchar weapon_button_up;
  weapon_button_up = TRUE;
}

void setup_start(void)
{
  int do_i_svg = -1, i_invuln = 0;

  empty_slate();

  player_struct.name[0] = 0;

  for (int i = 0; i < 4; i++)
    player_struct.difficulty[i] = 2;

  MacTuneKillCurrentTheme();

  // Check to see whether or not to play the intro cut scene
#ifdef GAMEONLY
  load_savegame_names();
#endif

  save_game_exists = (valid_save != 0);

  if (setup_mode != SETUP_CREDITS)
  {
    if (!save_game_exists && start_first_time)
    {
      play_intro_anim = TRUE;
    }
    else
    {
      play_intro_anim = FALSE;
    }
    setup_mode = SETUP_JOURNEY;
  }

  if (!start_first_time) closedown_game(TRUE);
  start_first_time = FALSE;

#ifdef GADGET
  // got rid of pointer type mismatch since one was a region and the other a gadget
  // someone should probably go and figure it out
  _current_root = NULL;
#endif
  _current_3d_flag = ANIM_UPDATE;
  _current_fr_context = NULL;
  _current_view = &setup_root_region;
  static_change_copy();
  message_info("");

#ifdef SVGA_SUPPORT
  change_svga_screen_mode();
#endif

  // clear the screen
  gr_clear(0);

  HotkeyContext = SETUP_CONTEXT;
  uiSetCurrentSlab(&setup_slab);

  // flush the keyboard and mouse - so we don't read old events
  kb_flush();
  mouse_flush();

  intro_num = ResOpenFile("res/data/intro.res");

  // slam in the right palette
  load_da_palette();

  if (do_i_svg != -1)
  {
#ifdef PLAYTEST
    player_invulnerable = i_invuln;
#endif
    uiShowMouse(NULL);
  }
  else if (!play_intro_anim)
  {
    uiShowMouse(NULL);
    switch (setup_mode)
    {
      case SETUP_DIFFICULTY: difficulty_draw(TRUE); break;
      case SETUP_JOURNEY: journey_draw(0); break;
      default: break;
    }
    direct_into_cutscene = FALSE;
  }
  else
  {
    direct_into_cutscene = TRUE;

    play_cutscene(START_CUTSCENE, FALSE);
  }

  if (music_on) MacTuneLoadTheme("titloop", 0);

  CaptureMouse(false);
}



void setup_exit(void)
{
  ResCloseFile(intro_num);
  ResCloseFile(splash_num);

#ifdef PALFX_FADES
  if (pal_fx_on) palfx_fade_down();
  else
  {
    gr_set_fcolor(BLACK);
    gr_rect(0, 0, 320, 200);
  }
#endif

  // must get rid of mouse - to maintain hidden mouse after loop
  if (!direct_into_cutscene) uiHideMouse(NULL);

  direct_into_cutscene = FALSE;

  MacTuneKillCurrentTheme();
}
//*****************************************************************************
