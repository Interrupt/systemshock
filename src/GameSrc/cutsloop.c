/*

Copyright (C) 2018-2020 Shockolate Project

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

#include <SDL.h>

#include "Shock.h"
#include "cutsloop.h"
#include "mainloop.h"
#include "fullscrn.h"
#include "game_screen.h"
#include "gamescr.h"
#include "tools.h"
#include "statics.h"

#include "MacTune.h"
#include "gr2ss.h"

#include "afile.h"
#include "movie.h"

SDL_AudioStream *cutscene_audiostream = NULL;

static uint8_t *cutscene_audiobuffer = NULL;
static uint8_t *cutscene_audiobuffer_pos = NULL;
static int cutscene_audiobuffer_size; //in blocks of MOVIE_DEFAULT_BLOCKLEN



static int cutscene_filehandle;

static uiSlab cutscene_slab;
static LGRegion cutscene_root_region;

static Afile *amovie = NULL;
static Apalette cutscene_pal;
static grs_bitmap movie_bitmap;
static long next_time;

static bool is_first_frame;
static bool done_playing_movie;
static long start_time;
static long next_draw_time;

static char *cutscene_files[3] =
{
  "res/data/svgaintr.res",
  "res/data/svgadeth.res", 
  "res/data/svgaend.res"
};

static Ref cutscene_anims[3] =
{
  0xbd6,
  0xbd7,
  0xbd8
};



extern uchar sfx_on;
extern char which_lang;

extern bool UseCutscenePalette; //see Shock.c

//filled in amov.c when chunk contains subtitle data
extern char EngSubtitle[256];
extern char FrnSubtitle[256];
extern char GerSubtitle[256];

extern SDL_AudioDeviceID device;


void AudioStreamCallback(void *userdata, unsigned char *stream, int len)
{
  SDL_AudioStream *as = *(SDL_AudioStream **)userdata;

  if (as != NULL && SDL_AudioStreamAvailable(as) > 0)
    SDL_AudioStreamGet(as, stream, len);
}



uchar cutscene_key_handler(uiEvent *ev, LGRegion *r, intptr_t user_data)
{
  uiCookedKeyData *kd = &ev->cooked_key_data;
  int code = kd->code & ~(KB_FLAG_DOWN | KB_FLAG_2ND);

  if (kd->code & KB_FLAG_DOWN)
  {
    switch (code)
    {
      case KEY_ESC:
      case KEY_ENTER:
      case KEY_SPACE:
        // Go back to the main menu
        _new_mode = SETUP_LOOP;
        chg_set_flg(GL_CHG_LOOP);
      break;
    }
  }

  return TRUE;
}



uchar cutscene_mouse_handler(uiEvent *ev, LGRegion *r, intptr_t user_data)
{
    return TRUE;
}



void cutscene_start(void)
{
  DEBUG("Cutscene start");

#ifdef SVGA_SUPPORT
  change_svga_screen_mode();
#endif

  generic_reg_init(TRUE, &cutscene_root_region, NULL, &cutscene_slab, cutscene_key_handler, cutscene_mouse_handler);

  _current_view = &cutscene_root_region;
  uiSetCurrentSlab(&cutscene_slab);

  uiHideMouse(NULL);

  CaptureMouse(FALSE);
}



void cutscene_exit(void)
{
  DEBUG("Cutscene exit");

  if (cutscene_audiostream != NULL)
  {
    SDL_PauseAudioDevice(device, 1);
    SDL_Delay(1);

    SDL_FreeAudioStream(cutscene_audiostream);
    cutscene_audiostream = NULL;

    if (cutscene_audiobuffer)
    {
      free(cutscene_audiobuffer);
      cutscene_audiobuffer = NULL;
    }
  }

  if (cutscene_filehandle > 0) {ResCloseFile(cutscene_filehandle); cutscene_filehandle = 0;}

  if (movie_bitmap.bits != NULL) {free(movie_bitmap.bits); movie_bitmap.bits = NULL;}

  if (amovie != NULL) {free(amovie); amovie = NULL;}
}



void cutscene_loop(void)
{
  fix time;
  long cur_time = SDL_GetTicks();

  static uint8_t palette[3*256];

  if (cutscene_audiostream)
  {
    SDL_PauseAudioDevice(device, 0);
  
    if (cutscene_audiobuffer_size > 0)
    {
      // === adjust volume in buffer here ===

      SDL_AudioStreamPut(cutscene_audiostream, cutscene_audiobuffer_pos, MOVIE_DEFAULT_BLOCKLEN);
      cutscene_audiobuffer_pos += MOVIE_DEFAULT_BLOCKLEN;
      cutscene_audiobuffer_size--;
    }
  }

  if (is_first_frame)
  {
    is_first_frame = FALSE;
    next_time = cur_time;
    start_time = SDL_GetTicks();

    // Read the first frame
    AfileReadFullFrame(amovie, &movie_bitmap, &time);
    next_draw_time = start_time + fix_float(time) * 1000.0;

    // Set the initial palette
    memcpy(palette, amovie->v.pal.rgb, 3*256);

    gr_clear(0x00);
  }

  if (cur_time > next_draw_time)
  {
    // Get the palette for this frame, if any
    if (AfileGetFramePal(amovie, &cutscene_pal))
      memcpy(palette+3*cutscene_pal.index, cutscene_pal.rgb, 3*cutscene_pal.numcols);

    UseCutscenePalette = TRUE; //see Shock.c
    gr_set_pal(0, 256, palette);
    UseCutscenePalette = FALSE; //see Shock.c

    gr_set_fcolor(255);

    // Draw this frame
    gr_clear(0x00);

    float vscale = (float)amovie->v.height / (float)amovie->v.width;

    int offset = (320 - (amovie->v.width / 2)) / 2;
    ss_scale_bitmap(&movie_bitmap, offset, offset / 1.25, amovie->v.width / 2, amovie->v.height / 2);

    //draw subtitles

    char *buf = 0;
    switch (which_lang)
    {
      case 0: default: buf = EngSubtitle; break;
      case 1: buf = FrnSubtitle; break;
      case 2: buf = GerSubtitle; break;
    }

    if (buf && *buf)
    {
      short w, h, x, y;
      grs_font *fon = gr_get_font();
      gr_set_font((grs_font *)ResLock(RES_cutsceneFont));
      gr_string_size(buf, &w, &h);
      x = (320-w)/2;
      y = 158+(200-158-h)/2;
      ss_string(buf, x, y);
      ResUnlock(RES_cutsceneFont);
      gr_set_font(fon);
    }

    if (done_playing_movie)
    {
      UseCutscenePalette = TRUE; //see Shock.c
      extern void palfx_fade_down(void);
      palfx_fade_down();
      UseCutscenePalette = FALSE; //see Shock.c

      // Go back to the main menu
      _new_mode = SETUP_LOOP;
      chg_set_flg(GL_CHG_LOOP);
      return;
    }

    // Read the next frame
    if (AfileReadFullFrame(amovie, &movie_bitmap, &time) == -1)
    {
      DEBUG("Done playing movie!");
      done_playing_movie = TRUE;
      // Still want a bit of a delay before finishing
      next_draw_time += 5200;
    }
   	else next_draw_time = start_time + fix_float(time) * 1000.0;
  }
}



short play_cutscene(int id, bool show_credits)
{
  MacTuneKillCurrentTheme();

  cutscene_filehandle = ResOpenFile(cutscene_files[id]);
  if (cutscene_filehandle <= 0) {
      // If we failed to play the cutscene, go to setup / credits.
      _new_mode = SETUP_LOOP;
      chg_set_flg(GL_CHG_LOOP);
      return 0;
  }

  INFO("Playing Cutscene %i", id);

  *EngSubtitle = 0;
  *FrnSubtitle = 0;
  *GerSubtitle = 0;

  _new_mode = CUTSCENE_LOOP;
  chg_set_flg(GL_CHG_LOOP);

  is_first_frame = TRUE;
  done_playing_movie = FALSE;

  amovie = malloc(sizeof(Afile));
  memset(amovie, 0, sizeof(Afile));

  if (AfilePrepareRes(cutscene_anims[id], amovie) < 0)
  {
    WARN("%s: Cannot open Afile by id $%x", __FUNCTION__, cutscene_anims[id]);
    free(amovie);
    amovie = NULL;
    return ERR_FREAD;
  }

  cutscene_audiobuffer_size = AfileAudioLength(amovie);
  cutscene_audiobuffer = (uint8_t *)malloc(cutscene_audiobuffer_size * MOVIE_DEFAULT_BLOCKLEN);
  AfileGetAudio(amovie, cutscene_audiobuffer);

  AfileReadReset(amovie);

  if (sfx_on)
  {
    SDL_PauseAudioDevice(device, 1);
    SDL_Delay(1);

    cutscene_audiostream = SDL_NewAudioStream(AUDIO_U8, 1, fix_int(amovie->a.sampleRate), AUDIO_S16SYS, 2, 48000);

    cutscene_audiobuffer_pos = cutscene_audiobuffer;
  }

  return 1;
}
