#ifdef USE_OPENGL

#include <stdio.h>
#include "OpenGL.h"

#ifdef _WIN32
    #define GLEW_STATIC 1
    #include <SDL.h>
    #include <GL/glew.h>
#else
    #define GL_GLEXT_PROTOTYPES
    #ifdef __APPLE__
    #include <OpenGL/gl.h>
    #else
    #include <GL/gl.h>
    #include <GL/glext.h>
    #endif

    #include <SDL.h>
    #include <SDL_opengl.h>
#endif

extern "C" {
    #include "mainloop.h"
    #include "map.h"
    #include "frintern.h"
    #include "frflags.h"
    #include "player.h"
    #include "textmaps.h"
    #include "star.h"
    #include "tools.h"
    #include "Prefs.h"
    #include "Shock.h"
    #include "faketime.h"

    extern SDL_Renderer *renderer;
    extern SDL_Palette *sdlPalette;
    extern uint _fr_curflags;
}

#include <map>
#include <sstream>

struct CachedTexture {
    SDL_Surface *bitmap;
    SDL_Surface *converted;
    GLuint texture;
    long lastDrawTime;
    bool locked;
};

#define MAX_CACHED_TEXTURES 1024

static SDL_GLContext context;
static GLuint textureShaderProgram;
static GLuint colorShaderProgram;
static GLuint dynTexture;
static GLuint viewBackupTexture;

// Texture cache to keep SDL surfaces and GL textures in memory
static std::map<uint8_t *, CachedTexture> texturesByBitsPtr;

static float view_scale;
static int phys_width;
static int phys_height;
static int phys_offset_x;
static int phys_offset_y;

static int render_width;
static int render_height;

// View matrix; Z offset experimentally tweaked for near-perfect alignment
// between GL projection and software projection (sprite screen coordinates)
static const float ViewMatrix[] = {
    1.0, 0.0,   0.0, 0.0,
    0.0, 1.0,   0.0, 0.0,
    0.0, 0.0,   1.0, 0.0,
    0.0, 0.0, -0.01, 1.0
};

// Projection matrix; experimentally tweaked for near-perfect alignment:
// FOV 89.5 deg, aspect ratio 1:1, near plane 0, far plane 100
static const float ProjectionMatrix[] = {
    1.00876,     0.0,  0.0,  0.0,
        0.0, 1.00876,  0.0,  0.0,
        0.0,     0.0, -1.0, -1.0,
        0.0,     0.0,  0.0,  0.0
};

// Identity matrix for sprite rendering
static const float IdentityMatrix[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

static GLuint compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, NULL, buffer);
        ERROR("Error compiling shader!\n");
        puts(buffer);
        exit(1);
    }
    return shader;
}

static GLuint loadShader(GLenum type, const char *filename) {

    DEBUG("Loading shader %s", filename);

    char fb[256];
    sprintf(fb, "shaders/%s", filename);

    FILE* file = fopen(fb, "r");
    if(file == NULL) {
        ERROR("Error loading shader %s!\n", fb);
        exit(1);
    }

    std::stringstream source;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *c = &line[strlen(line) - 1];
        while (c >= line && (*c == '\r' || *c == '\n'))
            *(c--) = '\0';
        source << line << '\n';
    }

    return compileShader(type, source.str().c_str());
}

int init_opengl() {
    DEBUG("Initializing OpenGL");
    context = SDL_GL_CreateContext(window);

#ifdef _WIN32
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
#endif

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL, 0.05f);

    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, "main.vert");
    GLuint textureShader = loadShader(GL_FRAGMENT_SHADER, "texture.frag");
    GLuint colorShader = loadShader(GL_FRAGMENT_SHADER, "color.frag");

    textureShaderProgram = glCreateProgram();
    glAttachShader(textureShaderProgram, vertexShader);
    glAttachShader(textureShaderProgram, textureShader);
    glLinkProgram(textureShaderProgram);

    colorShaderProgram = glCreateProgram();
    glAttachShader(colorShaderProgram, vertexShader);
    glAttachShader(colorShaderProgram, colorShader);
    glLinkProgram(colorShaderProgram);

    glGenTextures(1, &dynTexture);
    glGenTextures(1, &viewBackupTexture);

    return 0;
}

void opengl_resize(int width, int height) {
    SDL_GL_MakeCurrent(window, context);

    int logical_width, logical_height;
    SDL_RenderGetLogicalSize(renderer, &logical_width, &logical_height);

    float scale_x = (float)width / logical_width;
    float scale_y = (float)height / logical_height;

    if (scale_x >= scale_y) {
        // physical aspect ratio is wider; black borders left and right
        view_scale = scale_y;
    } else {
        // physical aspect ratio is narrower; black borders at top and bottom
        view_scale = scale_x;
    }

    phys_width = view_scale * logical_width;
    phys_height = view_scale * logical_height;

    int border_x = width - phys_width;
    int border_y = height - phys_height;
    phys_offset_x = border_x / 2;
    phys_offset_y = border_y / 2;

    // allocate a buffer for the framebuffer backup
    glBindTexture(GL_TEXTURE_2D, viewBackupTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, phys_width, phys_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
}

bool use_opengl() {
    return gShockPrefs.doUseOpenGL &&
           (_current_loop == GAME_LOOP || _current_loop == FULLSCREEN_LOOP) &&
           !global_fullmap->cyber &&
           !(_fr_curflags & (FR_PICKUPM_MASK | FR_HACKCAM_MASK));
}

bool should_opengl_swap() {
    return gShockPrefs.doUseOpenGL &&
           (_current_loop == GAME_LOOP || _current_loop == FULLSCREEN_LOOP) &&
           !global_fullmap->cyber;
}

void opengl_backup_view() {
    // save the framebuffer into a texture after rendering the 3D view(s)
    // but before blitting the HUD overlay
    SDL_GL_MakeCurrent(window, context);

    glViewport(phys_offset_x, phys_offset_y, phys_width, phys_height);
    glBindTexture(GL_TEXTURE_2D, viewBackupTexture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, phys_offset_x, phys_offset_y, phys_width, phys_height);
}

void opengl_swap_and_restore() {
    // restore the view backup (without HUD overlay) for incremental
    // updates in the subsequent frame
    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SwapWindow(window);

    glViewport(phys_offset_x, phys_offset_y, phys_width, phys_height);
    glUseProgram(textureShaderProgram);

    GLint uniView = glGetUniformLocation(textureShaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, IdentityMatrix);

    GLint uniProj = glGetUniformLocation(textureShaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, IdentityMatrix);

    GLint tcAttrib = glGetAttribLocation(textureShaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(textureShaderProgram, "light");
    glBindTexture(GL_TEXTURE_2D, viewBackupTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBegin(GL_TRIANGLE_STRIP);
    glVertexAttrib2f(tcAttrib, 1.0f, 0.0f);
    glVertexAttrib1f(lightAttrib, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glVertexAttrib2f(tcAttrib, 1.0f, 1.0f);
    glVertexAttrib1f(lightAttrib, 1.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);
    glVertexAttrib2f(tcAttrib, 0.0f, 0.0f);
    glVertexAttrib1f(lightAttrib, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glVertexAttrib2f(tcAttrib, 0.0f, 1.0f);
    glVertexAttrib1f(lightAttrib, 1.0f);
    glVertex3f(-1.0f, 1.0f, 0.0f);
    glEnd();

    // check OpenGL error
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        ERROR("OpenGL error: %i", err);
}

void toggle_opengl() {
    if (!gShockPrefs.doUseOpenGL) {
        message_info("OpenGL rendering, no texture filter");
        gShockPrefs.doUseOpenGL = true;
        gShockPrefs.doLinearScaling = false;
    } else if (!gShockPrefs.doLinearScaling) {
        message_info("OpenGL rendering, linear texture filter");
        gShockPrefs.doLinearScaling = true;
    } else {
        message_info("Software rendering");
        gShockPrefs.doUseOpenGL = false;
    }
    SavePrefs();
}

void opengl_set_viewport(int x, int y, int width, int height) {
    render_width = width;
    render_height = height;

    SDL_GL_MakeCurrent(window, context);
    int scaled_width = width * view_scale;
    int scaled_height = height * view_scale;
    int scaled_x = phys_offset_x + x * view_scale;
    int scaled_y = phys_offset_y + phys_height - scaled_height - y * view_scale;
    glViewport(scaled_x, scaled_y, scaled_width, scaled_height);
}

static bool opengl_cache_texture(CachedTexture toCache, grs_bitmap *bm) {
    SDL_GL_MakeCurrent(window, context);

    if(texturesByBitsPtr.size() < MAX_CACHED_TEXTURES) {
        // We have enough room, generate the new texture
        glGenTextures(1, &toCache.texture);
        glBindTexture(GL_TEXTURE_2D, toCache.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, toCache.converted->pixels);

        texturesByBitsPtr[bm->bits] = toCache;
        return true;
    }

    // Not enough room, just use the dynTexture
    glBindTexture(GL_TEXTURE_2D, dynTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, toCache.converted->pixels);

    return false;
}

static CachedTexture* opengl_get_texture(grs_bitmap *bm) {
    std::map<uint8_t *, CachedTexture>::iterator iter = texturesByBitsPtr.find(bm->bits);
    if (iter != texturesByBitsPtr.end()) {
        return &iter->second;
    }
    return NULL;
}

void opengl_clear_texture_cache() {
    if(texturesByBitsPtr.size() == 0)
        return;

    DEBUG("Clearing OpenGL texture cache.");

    SDL_GL_MakeCurrent(window, context);

    std::map<uint8_t *, CachedTexture>::iterator iter;
    for(iter = texturesByBitsPtr.begin(); iter != texturesByBitsPtr.end(); iter++) {
        CachedTexture ct = iter->second;
        if(!ct.locked) { // don't free locked surfaces
            SDL_FreeSurface(ct.bitmap);
            SDL_FreeSurface(ct.converted);

            glDeleteTextures(1, &ct.texture);

            texturesByBitsPtr.erase(iter);
        }
    }
}

static SDL_Palette *createPalette(bool transparent) {
    SDL_Palette *palette = SDL_AllocPalette(256);
    SDL_SetPaletteColors(palette, sdlPalette->colors, 0, 256);

    // color 0: black or transparent
    if (transparent)
        palette->colors[0].a = 0x00;
    for (int i = 1; i < 256; i++) {
        // colors 1..31, except 2: always at maximum light level
        // colors 2, 32..255: no minimum brightness, use interpolated vertex light level
        // encode emissive property in the top half of the alpha color, since we don't use those bytes
        if (i < 32 && i != 2)
            palette->colors[i].a = 0xff;
        else
            palette->colors[i].a = 0x7f;
    }

    return palette;
}

static void convert_texture(grs_bitmap *bm, bool locked) {
    SDL_Surface *surface;
    if (bm->type == BMT_RSD8) {
        grs_bitmap decoded;
        gr_rsd8_convert(bm, &decoded);
        surface = SDL_CreateRGBSurfaceFrom(decoded.bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    } else {
        surface = SDL_CreateRGBSurfaceFrom(bm->bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    }

    SDL_Palette *palette = createPalette(bm->flags & BMF_TRANS);
    SDL_SetSurfacePalette(surface, palette);
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreePalette(palette);

    // Cache this new surface.
    CachedTexture ct;
    ct.bitmap = surface;
    ct.converted = rgba;
    ct.lastDrawTime = *tmd_ticks;
    ct.locked = locked;

    bool cached = opengl_cache_texture(ct, bm);
    if(!cached) {
        DEBUG("Not enough room to cache texture!");
        SDL_FreeSurface(surface);
        SDL_FreeSurface(rgba);
    }
}

void opengl_cache_texture(int idx, int size, grs_bitmap *bm) {
    if (idx < NUM_LOADED_TEXTURES) {
        CachedTexture* t = opengl_get_texture(bm);
        if(t == NULL)
            convert_texture(bm, true);
    }
}

static void set_texture(grs_bitmap *bm) {
    CachedTexture* t = opengl_get_texture(bm);
    if(t == NULL) {
        // Not cached, have to make it
        convert_texture(bm, false);
        return;
    }

    bool isDirty = false;

    if(t->locked) {
        if(t->lastDrawTime != *tmd_ticks) {
            // Locked surfaces only need to update their palettes once per frame

            SDL_Palette *palette = createPalette(bm->flags & BMF_TRANS);
            SDL_SetSurfacePalette(t->bitmap, palette);
            SDL_BlitSurface(t->bitmap, NULL, t->converted, NULL);
            SDL_FreePalette(palette);

            isDirty = true;
        }
    }
    else {
        if (bm->type == BMT_RSD8) {
            grs_bitmap decoded;
            gr_rsd8_convert(bm, &decoded);
            SDL_memmove(t->bitmap->pixels, decoded.bits, bm->w * bm->h);
        }
        else {
            SDL_memmove(t->bitmap->pixels, bm->bits, bm->w * bm->h);
        }

        SDL_Palette *palette = createPalette(bm->flags & BMF_TRANS);
        SDL_SetSurfacePalette(t->bitmap, palette);
        SDL_BlitSurface(t->bitmap, NULL, t->converted, NULL);
        SDL_FreePalette(palette);

        isDirty = true;
    }

    glBindTexture(GL_TEXTURE_2D, t->texture);
    t->lastDrawTime = *tmd_ticks;

    if(isDirty) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->converted->pixels);   
    }
}

static void draw_vertex(const g3s_point& vertex, GLint tcAttrib, GLint lightAttrib) {

    // Default, per-vertex lighting
    float light = vertex.i / 4096.0f;

    // Could be a CLUT color instead, use that for lighting
    if(gr_get_fill_type() == FILL_CLUT) {
        // Ugly hack: We don't get the original light value, so we have to
        // recalculate it from the offset into the global lighting lookup
        // table.
        uchar* clut = (uchar*)gr_get_fill_parm();
        light = (clut - grd_screen->ltab) / 4096.0f;
    }

    glVertexAttrib2f(tcAttrib, vertex.uv.u / 256.0, vertex.uv.v / 256.0);
    glVertexAttrib1f(lightAttrib, 1.0f - light);
    glVertex3f(vertex.x / 65536.0f,  vertex.y / 65536.0f, -vertex.z / 65536.0f);
}

int opengl_draw_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) {
    return opengl_light_tmap(n, vp, bm);
}

int opengl_light_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) {
    if (n != 3 && n != 4) {
        WARN("Unexpected number of texture vertices (%d)", n);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);
    glUseProgram(textureShaderProgram);

    GLint uniView = glGetUniformLocation(textureShaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, ViewMatrix);

    GLint uniProj = glGetUniformLocation(textureShaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, ProjectionMatrix);

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint tcAttrib = glGetAttribLocation(textureShaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(textureShaderProgram, "light");

    glBegin(GL_TRIANGLE_STRIP);
    draw_vertex(*(vp[1]), tcAttrib, lightAttrib);
    draw_vertex(*(vp[0]), tcAttrib, lightAttrib);
    draw_vertex(*(vp[2]), tcAttrib, lightAttrib);
    if (n > 3)
        draw_vertex(*(vp[3]), tcAttrib, lightAttrib);
    glEnd();

    return CLIP_NONE;
}

static float convx(float x) {
    return  x/32768.0f / render_width - 1;
}

static float convy(float y) {
    return -y/32768.0f / render_height + 1;
}

int opengl_bitmap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_tmap_info *ti) {
    if (n != 4) {
        WARN("Unexpected number of bitmap vertices (%d)", n);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);
    glUseProgram(textureShaderProgram);

    GLint uniView = glGetUniformLocation(textureShaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, IdentityMatrix);

    GLint uniProj = glGetUniformLocation(textureShaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, IdentityMatrix);

    GLint tcAttrib = glGetAttribLocation(textureShaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(textureShaderProgram, "light");

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float light = 1.0f;
    if (ti->flags & TMF_CLUT) {
        // Ugly hack: We don't get the original 'i' value, so we have to
        // recalculate it from the offset into the global lighting lookup
        // table.
        light = 1.0 - (ti->clut - grd_screen->ltab) / 4096.0f;
    }

    glBegin(GL_TRIANGLE_STRIP);
    glVertexAttrib2f(tcAttrib, 1.0f, 0.0f);
    glVertexAttrib1f(lightAttrib, light);
    glVertex3f(convx(vpl[1]->x), convy(vpl[1]->y), 0.0f);
    glVertexAttrib2f(tcAttrib, 0.0f, 0.0f);
    glVertexAttrib1f(lightAttrib, light);
    glVertex3f(convx(vpl[0]->x), convy(vpl[0]->y), 0.0f);
    glVertexAttrib2f(tcAttrib, 1.0f, 1.0f);
    glVertexAttrib1f(lightAttrib, light);
    glVertex3f(convx(vpl[2]->x), convy(vpl[2]->y), 0.0f);
    glVertexAttrib2f(tcAttrib, 0.0f, 1.0f);
    glVertexAttrib1f(lightAttrib, light);
    glVertex3f(convx(vpl[3]->x), convy(vpl[3]->y), 0.0f);
    glEnd();

    return CLIP_NONE;
}

static void set_color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    const uint8_t pixel[] = { red, green, blue, alpha };
    glBindTexture(GL_TEXTURE_2D, dynTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

int opengl_draw_poly(long c, int n_verts, g3s_phandle *p, char gour_flag) {
    if (n_verts < 3) {
        WARN("Unexpected number of polygon vertices (%d)", n_verts);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);
    glUseProgram(colorShaderProgram);

    GLint uniView = glGetUniformLocation(colorShaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, ViewMatrix);

    GLint uniProj = glGetUniformLocation(colorShaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, ProjectionMatrix);

    if (gour_flag == 1 || gour_flag == 3) {
        // translucent; see init_pal_fx() for translucency parameters
        switch (c) {
            case 247: set_color(120, 120, 120,  80); break; // dark fog
            case 248: set_color(170, 170, 170,  80); break; // medium fog
            case 249: set_color(255,   0,   0,  80); break; // red fog
            case 250: set_color(  0, 255,   0,  80); break; // green fog
            case 251: set_color(  0,   0, 255,  80); break; // blue fog
            case 252: set_color(240, 240, 240,  80); break; // light fog
            case 253: set_color(  0,   0, 255, 128); break; // blue force field
            case 254: set_color(  0, 255,   0, 128); break; // green force field
            case 255: set_color(255,   0,   0, 128); break; // red force field
            default: return CLIP_ALL;
        }
    } else if (c == 255) {
        // transparent
        return CLIP_NONE;
    } else {
        // solid color
        SDL_Color color = sdlPalette->colors[c];
        set_color(color.r, color.g, color.b, 255);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint tcAttrib = glGetAttribLocation(colorShaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(colorShaderProgram, "light");

    long a = 0, b = n_verts - 1;
    glBegin(GL_TRIANGLE_STRIP);
    while (true) {
        draw_vertex(*(p[a]), tcAttrib, lightAttrib);
        if (a++ == b) break;
        draw_vertex(*(p[b]), tcAttrib, lightAttrib);
        if (b-- == a) break;
    }
    glEnd();

    return CLIP_NONE;
}

void opengl_set_stencil(int v) {
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, v, ~0);
}

void opengl_begin_stars() {
    SDL_GL_MakeCurrent(window, context);

    glPointSize(2.5f);
    glEnable(GL_POINT_SMOOTH);

    GLint uniView = glGetUniformLocation(textureShaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, ViewMatrix);

    GLint uniProj = glGetUniformLocation(textureShaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, ProjectionMatrix);

    // Only draw stars where the stencil value is 0xFF (Sky!)
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_EQUAL, 0xFF, ~0);
}

void opengl_end_stars() {
    // Turn off the stencil test
    opengl_set_stencil(0x00);
}

int opengl_draw_star(long c, int n_verts, g3s_phandle *p) {
    SDL_GL_MakeCurrent(window, context);

    GLint tcAttrib = glGetAttribLocation(textureShaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(textureShaderProgram, "light");

    g3s_point& vertex = *(p[0]);

    set_color(200, 200, 200, 255);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gShockPrefs.doLinearScaling ? GL_LINEAR : GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // FIXME: Might be able to draw all the stars at once, in one Begin / End!
    glBegin(GL_POINTS);
    glVertexAttrib2f(tcAttrib, 0.1f, 0.1f);
    glVertexAttrib1f(lightAttrib, 1.0f);
    glVertex3f(vertex.x / 65536.0f,  vertex.y / 65536.0f, -vertex.z / 65536.0f);
    glEnd();

    return CLIP_NONE;
}

void opengl_clear() {
    SDL_GL_MakeCurrent(window, context);

    // Make sure everything starts with a stencil of 0xFF
    glClearStencil(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);

    // Draw everything with a stencil of 0
    opengl_set_stencil(0x00);
}

#endif // USE_OPENGL
