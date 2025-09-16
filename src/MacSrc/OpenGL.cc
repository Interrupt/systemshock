#ifdef USE_OPENGL

#include <cstdio>
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
#include "render.h"
#include "wares.h"

extern SDL_Renderer *renderer;
extern SDL_Palette *sdlPalette;
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

struct Shader {
    GLuint shaderProgram;
    GLint uniView;
    GLint uniProj;
    GLint uniNightSight;
    GLint uniMutant;
    GLint tcAttrib;
    GLint lightAttrib;
    GLint colorAttrib;
};

struct FrameBuffer {
    GLuint frameBuffer;
    GLuint stencilBuffer;
    GLuint texture;
    int width;
    int height;
};

#define MAX_CACHED_TEXTURES 1024

static Shader textureShaderProgram;
static Shader colorShaderProgram;
static Shader starShaderProgram;

static FrameBuffer backupBuffer;

static SDL_GLContext context;
static GLuint dynTexture;

static SDL_Palette *opaquePalette;
static SDL_Palette *transparentPalette;

// Texture cache to keep SDL surfaces and GL textures in memory
static std::map<uint64_t, CachedTexture> texturesByBitsPtr;

static float view_scale;
static int phys_width;
static int phys_height;
static int phys_offset_x;
static int phys_offset_y;

static int render_width;
static int render_height;

static bool opengl_enabled = true;
static bool palette_dirty = false;
static bool blend_enabled = true;
static GLuint bound_texture = -1;

// View matrix; Z offset experimentally tweaked for near-perfect alignment
// between GL projection and software projection (sprite screen coordinates)
static const float ViewMatrix[] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, -0.01, 1.0};

// Projection matrix; experimentally tweaked for near-perfect alignment:
// FOV 89.5 deg, aspect ratio 1:1, near plane 0, far plane 100
static const float ProjectionMatrix[] = {1.00876, 0.0, 0.0,  0.0,  0.0, 1.00876, 0.0, 0.0,
                                         0.0,     0.0, -1.0, -1.0, 0.0, 0.0,     0.0, 0.0};

// Identity matrix for sprite rendering
static const float IdentityMatrix[] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

static inline GLint get_texture_min_func() {
    // convert prefs texture filtering mode to GL min func value
    switch (gShockPrefs.doTextureFilter) {
    case 0:
        return GL_NEAREST;
    case 1:
        return GL_LINEAR;
        //        case 2: return GL_LINEAR_MIPMAP_LINEAR;
    }

    WARN("gShockPrefs.doTextureFilter=%d is invalid; resetting to zero (unflitered)", gShockPrefs.doTextureFilter);
    gShockPrefs.doTextureFilter = 0;
    return GL_NEAREST;
}

static inline GLint get_texture_mag_func() {
    // convert prefs texture filtering mode to GL mag func value
    switch (gShockPrefs.doTextureFilter) {
    case 0:
        return GL_NEAREST;
    case 1:
        return GL_LINEAR;
    case 2:
        return GL_LINEAR;
    }

    WARN("gShockPrefs.doTextureFilter=%d is invalid; resetting to zero (unflitered)", gShockPrefs.doTextureFilter);
    gShockPrefs.doTextureFilter = 0;
    return GL_NEAREST;
}

static void set_blend_mode(bool enabled) {
    // change the blend mode, if not set already
    if (blend_enabled != enabled) {
        blend_enabled = enabled;
        if (blend_enabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
    }
}

static void bind_texture(GLuint tex) {
    // bind a texture, if not bound already
    if (bound_texture != tex) {
        bound_texture = tex;
        glBindTexture(GL_TEXTURE_2D, bound_texture);
    }
}

static GLuint compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, nullptr, buffer);
        ERROR("Error compiling shader: %s", buffer);
        return 0;
    }

    return shader;
}

static GLuint loadShader(GLenum type, const char *filename) {

    DEBUG("Loading shader %s", filename);

#ifdef __APPLE__
    char fb[1024];
    sprintf(fb, "%s/shaders/%s", SDL_GetBasePath(), filename);
#else
    char fb[256];
    sprintf(fb, "shaders/%s", filename);
#endif

    FILE *file = fopen(fb, "r");
    if (file == nullptr) {
        ERROR("Could not open shader file %s!", fb);
        return 0;
    }

    std::stringstream source;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *c = &line[strlen(line) - 1];
        while (c >= line && (*c == '\r' || *c == '\n'))
            *(c--) = '\0';
        source << line << '\n';
    }
    fclose(file);

    GLuint s = compileShader(type, source.str().c_str());

    if (s == 0) {
        ERROR("Could not compile shader %s", filename);
    }
    return s;
}

static int CreateShader(const char *vertexShaderFile, const char *fragmentShaderFile, Shader *outShader) {
    GLuint vertShader = loadShader(GL_VERTEX_SHADER, vertexShaderFile);
    GLuint fragShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderFile);

    if (vertShader == 0 || fragShader == 0) {
        ERROR("Could not create shader %s : %s", vertexShaderFile, fragmentShaderFile);
        context = nullptr;
        return 1; // Error!
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertShader);
    glAttachShader(shaderProgram, fragShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    outShader->shaderProgram = shaderProgram;
    outShader->uniView = glGetUniformLocation(shaderProgram, "view");
    outShader->uniProj = glGetUniformLocation(shaderProgram, "proj");
    outShader->uniNightSight = glGetUniformLocation(shaderProgram, "nightsight");
    outShader->uniMutant = glGetUniformLocation(shaderProgram, "mutant");
    outShader->tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    outShader->lightAttrib = glGetAttribLocation(shaderProgram, "light");
    outShader->colorAttrib = glGetAttribLocation(shaderProgram, "color");

    glUniformMatrix4fv(outShader->uniView, 1, false, IdentityMatrix);
    glUniformMatrix4fv(outShader->uniProj, 1, false, IdentityMatrix);

    return 0;
}

static FrameBuffer CreateFrameBuffer(int width, int height) {
    FrameBuffer newBuffer{};
    newBuffer.width = width;
    newBuffer.height = height;

    // Make a frame buffer, texture for color, and render buffer for stencil
    glGenFramebuffers(1, &newBuffer.frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, newBuffer.frameBuffer);

    glGenRenderbuffers(1, &newBuffer.stencilBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, newBuffer.stencilBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, newBuffer.stencilBuffer);

    glGenTextures(1, &newBuffer.texture);
    glBindTexture(GL_TEXTURE_2D, newBuffer.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, newBuffer.texture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        ERROR("Could not make FrameBuffer!: %x \n", status);
        context = nullptr;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return newBuffer;
}

static void BindFrameBuffer(FrameBuffer *buffer) {
    if (buffer == nullptr) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, buffer->frameBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, buffer->stencilBuffer);
        glViewport(0, 0, buffer->width, buffer->height);
    }
}

int init_opengl() {
    DEBUG("Initializing OpenGL");

    // Are we running in OpenGL mode?
    if (SDL_GL_GetCurrentContext() == nullptr) {
        ERROR("No OpenGL context! Falling back to Software mode.");
        return 1;
    }

    // Can we create the world rendering context?
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        ERROR("Could not create an OpenGL context! Falling back to Software mode.");
        return 1;
    }

#ifdef _WIN32
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
#endif

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_POINT_SPRITE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glAlphaFunc(GL_GEQUAL, 0.05f);

    CreateShader("main.vert", "texture.frag", &textureShaderProgram);
    CreateShader("main.vert", "color.frag", &colorShaderProgram);
    CreateShader("main.vert", "star.frag", &starShaderProgram);

    glGenTextures(1, &dynTexture);

    // Did the setup go okay?
    if (context == nullptr) {
        ERROR("OpenGL could not be initialized, falling back to Software mode.");
        return 1;
    }

    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    opengl_resize(width, height);

    // Now make the palettes
    opaquePalette = SDL_AllocPalette(256);
    transparentPalette = SDL_AllocPalette(256);

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

    backupBuffer = CreateFrameBuffer(logical_width, logical_height);

    INFO("OpenGL Resize %i %i %i %i", width, height, phys_width, phys_height);

    // Redraw the options menu background in the new resolution
    extern uchar wrapper_screenmode_hack;
    if (wrapper_screenmode_hack) {
        render_run();
        wrapper_screenmode_hack = false;
    }
}

void opengl_change_palette() { palette_dirty = true; }

bool can_use_opengl() { return context != nullptr; }

bool use_opengl() {
    return can_use_opengl() && gShockPrefs.doUseOpenGL && opengl_enabled &&
           (_current_loop == GAME_LOOP || _current_loop == FULLSCREEN_LOOP) && !global_fullmap->cyber &&
           !(_fr_curflags & (FR_PICKUPM_MASK | FR_HACKCAM_MASK));
}

bool should_opengl_swap() {
    return can_use_opengl() && gShockPrefs.doUseOpenGL && opengl_enabled &&
           (_current_loop == GAME_LOOP || _current_loop == FULLSCREEN_LOOP) && !global_fullmap->cyber;
}

void opengl_end_frame() {
    SDL_GL_MakeCurrent(window, context);

    // Done rendering to the frame buffer, reset back to normal
    BindFrameBuffer(nullptr);
    palette_dirty = false;
}

static void updatePalette(SDL_Palette *palette, bool transparent) {
    // Update from the base
    SDL_SetPaletteColors(palette, sdlPalette->colors, 0, 256);

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
}

static bool nightsight_active() { return WareActive(player_struct.hardwarez_status[HARDWARE_GOGGLE_INFRARED]); }

void opengl_start_frame() {
    SDL_GL_MakeCurrent(window, context);

    // Start rendering to our frame buffer canvas
    BindFrameBuffer(&backupBuffer);

    // Setup the render width
    int logical_width, logical_height;
    SDL_RenderGetLogicalSize(renderer, &logical_width, &logical_height);

    render_height = logical_height;
    render_width = logical_width;

    // Update the palettes for this frame
    updatePalette(opaquePalette, false);
    updatePalette(transparentPalette, true);
}

void get_hdpi_scaling(int *x_scale, int *y_scale) {
    // We may need to scale up our OpenGL output to match some HDPI scaling
    int output_width, output_height;
    SDL_GetRendererOutputSize(renderer, &output_width, &output_height);

    int screen_width, screen_height;
    SDL_GetWindowSize(window, &screen_width, &screen_height);

    *x_scale = output_width / screen_width;
    *y_scale = output_height / screen_height;
}

void opengl_swap_and_restore(SDL_Surface *ui) {
    // restore the view backup (without HUD overlay) for incremental
    // updates in the subsequent frame
    SDL_GL_MakeCurrent(window, context);
    glClear(GL_COLOR_BUFFER_BIT);

    int x_hdpi_scale, y_hdpi_scale;
    get_hdpi_scaling(&x_hdpi_scale, &y_hdpi_scale);

    // Set the drawable area for the 3d view
    glViewport(phys_offset_x * x_hdpi_scale, phys_offset_y * y_hdpi_scale, phys_width * x_hdpi_scale,
               phys_height * y_hdpi_scale);
    set_blend_mode(false);

    // Bind and setup our general shader program
    glUseProgram(textureShaderProgram.shaderProgram);
    GLint tcAttrib = textureShaderProgram.tcAttrib;
    GLint lightAttrib = textureShaderProgram.lightAttrib;

    glUniform1i(textureShaderProgram.uniNightSight, nightsight_active());

    glUniformMatrix4fv(textureShaderProgram.uniView, 1, false, IdentityMatrix);
    glUniformMatrix4fv(textureShaderProgram.uniProj, 1, false, IdentityMatrix);

    // Draw the frame buffer to the screen as a quad
    bind_texture(backupBuffer.texture);
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

    // Finish drawing the 3d view
    glFlush();

    glUniform1i(textureShaderProgram.uniNightSight, false);

    // Check for OpenGL errors that might have happened
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        ERROR("OpenGL error: %i", err);

    // Blit the UI canvas over the 3d view
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, ui);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_DestroyTexture(texture);

    // Finally, swap to the screen
    SDL_RenderPresent(renderer);
}

void toggle_opengl() {
    if (gShockPrefs.doUseOpenGL) {
        switch (gShockPrefs.doTextureFilter) {
        case 0: {
            message_info("Switching to OpenGL bilinear rendering");
            gShockPrefs.doTextureFilter = 1;
        } break;
        case 1: {
            message_info("Switching to sofware rendering");
            gShockPrefs.doUseOpenGL = false;
            gShockPrefs.doTextureFilter = 0;
        } break;
        }
    } else {
        message_info("Switching to OpenGL unfiltered");
        gShockPrefs.doUseOpenGL = true;
        gShockPrefs.doTextureFilter = 0;
    }
    SavePrefs();
}

void opengl_set_viewport(int x, int y, int width, int height) {
    render_width = width;
    render_height = height;

    SDL_GL_MakeCurrent(window, context);

    int lw, lh;
    SDL_RenderGetLogicalSize(renderer, &lw, &lh);

    int draw_y = lh - height - y;
    glViewport(x, draw_y, width, height);

    // Make sure everything starts with a stencil of 0xFF
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, draw_y, width, height);
    glClearStencil(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    // Draw everything with a stencil of 0
    opengl_set_stencil(0x00);
}

static bool opengl_cache_texture(CachedTexture toCache, grs_bitmap *bm) {
    SDL_GL_MakeCurrent(window, context);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, bm->row);

    if (texturesByBitsPtr.size() < MAX_CACHED_TEXTURES) {
        // We have enough room, generate the new texture
        glGenTextures(1, &toCache.texture);
        bind_texture(toCache.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, toCache.converted->pixels);

        texturesByBitsPtr[(uint64_t)bm->bits | ((uint64_t)bm->w << 32u) | ((uint64_t)bm->h << 48u)] = toCache;
        return true;
    }

    // Not enough room, just use the dynTexture
    bind_texture(dynTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, toCache.converted->pixels);

    return false;
}

static CachedTexture *opengl_get_texture(grs_bitmap *bm) {
    auto iter = texturesByBitsPtr.find((uint64_t)bm->bits | ((uint64_t)bm->w << 32u) | ((uint64_t)bm->h << 48u));
    if (iter != texturesByBitsPtr.end()) {
        return &iter->second;
    }
    return nullptr;
}

void opengl_clear_texture_cache() {
    if (texturesByBitsPtr.empty())
        return;

    DEBUG("Clearing OpenGL texture cache.");

    SDL_GL_MakeCurrent(window, context);

    for (auto iter = texturesByBitsPtr.begin(); iter != texturesByBitsPtr.end();) {
        // don't free locked surfaces
        if (!iter->second.locked) {
            SDL_FreeSurface(iter->second.bitmap);
            SDL_FreeSurface(iter->second.converted);
            glDeleteTextures(1, &iter->second.texture);

            iter = texturesByBitsPtr.erase(iter);
        } else {
            iter++;
        }
    }
}

static void setPaletteForBitmap(grs_bitmap *bm, SDL_Surface *surface) {
    if (bm->flags & BMF_TRANS)
        SDL_SetSurfacePalette(surface, transparentPalette);
    else
        SDL_SetSurfacePalette(surface, opaquePalette);
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

    setPaletteForBitmap(bm, surface);
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);

    // Cache this new surface.
    CachedTexture ct{};
    ct.bitmap = surface;
    ct.converted = rgba;
    ct.lastDrawTime = *tmd_ticks;
    ct.locked = locked;

    bool cached = opengl_cache_texture(ct, bm);
    if (!cached) {
        DEBUG("Not enough room to cache texture!");
        SDL_FreeSurface(surface);
        SDL_FreeSurface(rgba);
    }
}

void opengl_cache_wall_texture(int idx, int size, grs_bitmap *bm) {
    if (idx < NUM_LOADED_TEXTURES) {
        CachedTexture *t = opengl_get_texture(bm);
        if (t == nullptr) {
            convert_texture(bm, true);
        } else {
            // Need to refresh this texture
            t->lastDrawTime = -1;
            palette_dirty = true;
        }
    }
}

static void set_texture(grs_bitmap *bm) {
    CachedTexture *t = opengl_get_texture(bm);
    if (t == nullptr) {
        // Not cached, have to make it
        convert_texture(bm, false);
        return;
    }

    bool isDirty = false;

    if (t->locked) {
        // Locked surfaces only need to update once
        if (palette_dirty && t->lastDrawTime != *tmd_ticks) {
            SDL_SetSurfacePalette(t->bitmap, transparentPalette); // Walls should show stars
            SDL_BlitSurface(t->bitmap, nullptr, t->converted, nullptr);

            isDirty = true;
        }
    } else {
        if (bm->type == BMT_RSD8) {
            grs_bitmap decoded;
            gr_rsd8_convert(bm, &decoded);
            SDL_memmove(t->bitmap->pixels, decoded.bits, bm->w * bm->h);
        } else {
            SDL_memmove(t->bitmap->pixels, bm->bits, bm->w * bm->h);
        }

        setPaletteForBitmap(bm, t->bitmap);
        SDL_BlitSurface(t->bitmap, nullptr, t->converted, nullptr);

        isDirty = true;
    }

    bind_texture(t->texture);
    t->lastDrawTime = *tmd_ticks;

    if (isDirty) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, bm->row);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->converted->pixels);
    }
}

static void draw_vertex(const g3s_point &vertex, GLint tcAttrib, GLint lightAttrib) {

    // Default, per-vertex lighting
    float light = 1.0f - (vertex.i / 4096.0f);

    if (nightsight_active()) {
        light = 1.0f;
    } else if (gr_get_fill_type() == FILL_CLUT) {
        // Could be a CLUT color instead, use that for lighting
        // Ugly hack: We don't get the original light value, so we have to
        // recalculate it from the offset into the global lighting lookup
        // table.
        auto *clut = (uint8_t *)gr_get_fill_parm();
        light = 1.0f - (clut - grd_screen->ltab) / 4096.0f;
    }

    if (tcAttrib >= 0)
        glVertexAttrib2f(tcAttrib, vertex.uv.u / 256.0, vertex.uv.v / 256.0);
    glVertexAttrib1f(lightAttrib, light);
    glVertex3f(vertex.x / 65536.0f, vertex.y / 65536.0f, -vertex.z / 65536.0f);
}

int opengl_draw_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) { return opengl_light_tmap(n, vp, bm); }

int opengl_light_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) {
    if (n != 3 && n != 4) {
        WARN("Unexpected number of texture vertices (%d)", n);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);
    set_blend_mode(bm->flags & BMF_TRANS);

    glUseProgram(textureShaderProgram.shaderProgram);

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, get_texture_min_func());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, get_texture_mag_func());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint tcAttrib = textureShaderProgram.tcAttrib;
    GLint lightAttrib = textureShaderProgram.lightAttrib;

    glUniformMatrix4fv(textureShaderProgram.uniView, 1, false, ViewMatrix);
    glUniformMatrix4fv(textureShaderProgram.uniProj, 1, false, ProjectionMatrix);

    glBegin(GL_TRIANGLE_STRIP);
    draw_vertex(*(vp[1]), tcAttrib, lightAttrib);
    draw_vertex(*(vp[0]), tcAttrib, lightAttrib);
    draw_vertex(*(vp[2]), tcAttrib, lightAttrib);
    if (n > 3)
        draw_vertex(*(vp[3]), tcAttrib, lightAttrib);
    glEnd();

    return CLIP_NONE;
}

static float convx(float x) { return x / 32768.0f / render_width - 1; }

static float convy(float y) { return -y / 32768.0f / render_height + 1; }

int opengl_bitmap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_tmap_info *ti) {
    if (n != 4) {
        WARN("Unexpected number of bitmap vertices (%d)", n);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);
    set_blend_mode(bm->flags & BMF_TRANS);

    glUseProgram(textureShaderProgram.shaderProgram);
    GLint tcAttrib = textureShaderProgram.tcAttrib;
    GLint lightAttrib = textureShaderProgram.lightAttrib;

    glUniformMatrix4fv(textureShaderProgram.uniView, 1, false, IdentityMatrix);
    glUniformMatrix4fv(textureShaderProgram.uniProj, 1, false, IdentityMatrix);

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, get_texture_min_func());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, get_texture_mag_func());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float light = 1.0f;
    if ((ti->flags & TMF_CLUT) && !nightsight_active()) {
        // Ugly hack: We don't get the original 'i' value, so we have to
        // recalculate it from the offset into the global lighting lookup
        // table.
        light = 1.0 - (ti->clut - grd_screen->ltab) / 4096.0f;
    }

    glUniform1i(textureShaderProgram.uniMutant, (bm->type == BMT_TLUC8) || (bm->flags == BMF_TLUC8));

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

    glUniform1i(textureShaderProgram.uniMutant, false);

    return CLIP_NONE;
}

static void set_color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    glVertexAttrib4f(colorShaderProgram.colorAttrib, red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
    set_blend_mode(alpha < 255);
}

int opengl_draw_poly(long c, int n_verts, g3s_phandle *p, char gour_flag) {
    if (n_verts < 3) {
        WARN("Unexpected number of polygon vertices (%d)", n_verts);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);

    glUseProgram(colorShaderProgram.shaderProgram);
    glUniformMatrix4fv(colorShaderProgram.uniView, 1, false, ViewMatrix);
    glUniformMatrix4fv(colorShaderProgram.uniProj, 1, false, ProjectionMatrix);

    if (gour_flag == 1 || gour_flag == 3) {
        // translucent; see init_pal_fx() for translucency parameters
        switch (c) {
        case 247:
            set_color(120, 120, 120, 80);
            break; // dark fog
        case 248:
            set_color(170, 170, 170, 80);
            break; // medium fog
        case 249:
            set_color(255, 0, 0, 80);
            break; // red fog
        case 250:
            set_color(0, 255, 0, 80);
            break; // green fog
        case 251:
            set_color(0, 0, 255, 80);
            break; // blue fog
        case 252:
            set_color(240, 240, 240, 80);
            break; // light fog
        case 253:
            set_color(0, 0, 255, 128);
            break; // blue force field
        case 254:
            set_color(0, 255, 0, 128);
            break; // green force field
        case 255:
            set_color(255, 0, 0, 128);
            break; // red force field
        default:
            return CLIP_ALL;
        }
    } else if (c == 255) {
        // transparent
        return CLIP_NONE;
    } else {
        // solid color
        SDL_Color color = sdlPalette->colors[c];
        set_color(color.r, color.g, color.b, 255);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, get_texture_min_func());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, get_texture_mag_func());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint lightAttrib = colorShaderProgram.lightAttrib;

    long a = 0, b = n_verts - 1;
    glBegin(GL_TRIANGLE_STRIP);
    while (true) {
        draw_vertex(*(p[a]), -1, lightAttrib);
        if (a++ == b)
            break;
        draw_vertex(*(p[b]), -1, lightAttrib);
        if (b-- == a)
            break;
    }
    glEnd();

    return CLIP_NONE;
}

void opengl_set_stencil(int v) {
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, v, ~0u);
}

void opengl_begin_stars() {
    SDL_GL_MakeCurrent(window, context);

    glPointSize(1.5 * (render_width / 320.0));

    glUseProgram(starShaderProgram.shaderProgram);
    glUniformMatrix4fv(starShaderProgram.uniView, 1, false, IdentityMatrix);
    glUniformMatrix4fv(starShaderProgram.uniProj, 1, false, IdentityMatrix);

    // Only draw stars where the stencil value is 0xFF (Sky!)
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_EQUAL, 0xFF, ~0u);

    set_blend_mode(true);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, get_texture_mag_func());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, get_texture_mag_func());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBegin(GL_POINTS);
}

void opengl_end_stars() {
    glEnd();

    // Turn off the stencil test and blending
    opengl_set_stencil(0x00);
    set_blend_mode(false);
}

int opengl_draw_star(fix star_x, fix star_y, int c, bool anti_alias) {
    SDL_GL_MakeCurrent(window, context);

    GLint lightAttrib = starShaderProgram.lightAttrib;

    float x = fix_float(star_x);
    float y = fix_float(star_y);

    // Lower screen resolutions should snap to pixels to avoid shimmering
    if (!anti_alias) {
        x = (int)x + 0.5f;
        y = (int)y + 0.5f;
    }

    x = (x / render_width) * 2.0 - 1.0;
    y = (y / render_height) * -2.0 + 1.0;

    // rescale the color so that it's 0..255, 0 = dark, 255 = light
    int std_color_base = 208;
    int std_color_range = 16;
    int color = (std_color_base + std_color_range - 1 - c);
    color = (255 * color) / (std_color_range + 1);

    glVertexAttrib1f(lightAttrib, color / 255.0f);
    glVertex3f(x, y, -0.25f);

    return CLIP_NONE;
}

void opengl_begin_sensaround(uchar version) {
    if (version == 1) {
        // Version 1 of the sensaround is old tech, and should render in software mode :D
        opengl_enabled = false;
    }
}

void opengl_end_sensaround() { opengl_enabled = true; }

#endif // USE_OPENGL
