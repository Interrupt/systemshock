#ifndef __MACSRC_OPENGL_H
#define __MACSRC_OPENGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <3d.h>

#ifdef USE_OPENGL

int init_opengl();
void opengl_cache_texture(int idx, int size, grs_bitmap *bm);

bool use_opengl();
void toggle_opengl();
void opengl_resize(int width, int height);
bool should_opengl_swap();

int opengl_draw_tmap(int n, g3s_phandle *vp, grs_bitmap *bm);
int opengl_light_tmap(int n, g3s_phandle *vp, grs_bitmap *bm);
int opengl_bitmap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_tmap_info *ti);
int opengl_draw_poly(long c, int n_verts, g3s_phandle *p, char gour_flag);
void opengl_clear();
int opengl_draw_star(long c, int n_verts, g3s_phandle *p);
void opengl_begin_stars();
void opengl_end_stars();
void opengl_set_stencil(int v);

#else

static int init_opengl() { return 0; }
static void opengl_cache_texture(int idx, int size, grs_bitmap *bm) {}

static bool use_opengl() { return false; }
static void toggle_opengl() {}
static void opengl_resize(int width, int height) {}
static bool should_opengl_swap() { return false; }

static int opengl_draw_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) { return 0; }
static int opengl_light_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) { return 0; }
static int opengl_bitmap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_tmap_info *ti) { return 0; }
static int opengl_draw_poly(long c, int n_verts, g3s_phandle *p, char gour_flag) { return 0; }
static void opengl_clear() {}
static int opengl_draw_star(long c, int n_verts, g3s_phandle *p) { return 0; }
static void opengl_begin_stars() { }
static void opengl_end_stars() { }
static void opengl_set_stencil(int v);

#endif

#ifdef __cplusplus
}
#endif

#endif
