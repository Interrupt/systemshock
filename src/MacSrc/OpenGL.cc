#ifdef USE_OPENGL

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
    #include "Shock.h"

    extern SDL_Renderer *renderer;
    extern SDL_Palette *sdlPalette;
    extern uint _fr_curflags;
}

#include <map>

bool _use_opengl = true;
int _blend_mode = GL_LINEAR;

static SDL_GLContext context;
static GLuint shaderProgram;
static GLuint dynTexture;
static GLuint viewBackupTexture;

// static cache for the most important textures;
// initialized during load_textures() in textmaps.c
static GLuint textures[NUM_LOADED_TEXTURES];
static std::map<uint8_t *, GLuint> texturesByBitsPtr;

static float view_scale;
static int phys_width;
static int phys_height;
static int phys_offset_x;
static int phys_offset_y;

static int render_width;
static int render_height;

static const char *VertexShader =
    "#version 110\n"
    "\n"
    "attribute vec2 texcoords;\n"
    "attribute float light;\n"
    "\n"
    "varying vec2 TexCoords;\n"
    "varying float Light;\n"
    "\n"
    "uniform mat4 view;\n"
    "uniform mat4 proj;\n"
    "\n"
    "void main() {\n"
    "    TexCoords = texcoords;\n"
    "    Light = light;\n"
    "    gl_Position = proj * view * gl_Vertex;\n"
    "}\n";

static const char *FragmentShader =
    "#version 110\n"
    "\n"
    "varying vec2 TexCoords;\n"
    "varying float Light;\n"
    "\n"
    "uniform sampler2D tex;\n"
    "\n"
    "void main() {\n"
    "    vec4 t = texture2D(tex, TexCoords);\n"
    "    gl_FragColor = vec4(t.r * Light, t.g * Light, t.b * Light, t.a);\n"
    "}\n";

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
        printf("Error compiling shader!\n");
        puts(buffer);
        exit(1);
    }
    return shader;
}

int init_opengl() {
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

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FragmentShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glGenTextures(NUM_LOADED_TEXTURES, textures);
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
    printf("window = %dx%d, scale = %.2f, viewport = %dx%d, offset = %d/%d\n",
           width, height, view_scale, phys_width, phys_height, phys_offset_x, phys_offset_y);

    // allocate a buffer for the framebuffer backup
    glBindTexture(GL_TEXTURE_2D, viewBackupTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, phys_width, phys_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
}

bool use_opengl() {
    return _use_opengl &&
           (_current_loop == GAME_LOOP || _current_loop == FULLSCREEN_LOOP) &&
           !global_fullmap->cyber &&
           !(_fr_curflags & (FR_PICKUPM_MASK | FR_HACKCAM_MASK));
}

bool should_opengl_swap() {
    return _use_opengl &&
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

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, IdentityMatrix);

    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, IdentityMatrix);

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(shaderProgram, "light");
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
    if(_use_opengl && _blend_mode == GL_LINEAR) {
        _blend_mode = GL_NEAREST;
    }
    else {
        _use_opengl = !_use_opengl;

        if(_use_opengl == TRUE) {
            _blend_mode = GL_LINEAR;
        }
    }
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

static void convert_texture(GLuint texture, grs_bitmap *bm) {
    SDL_Surface *surface;
    if (bm->type == BMT_RSD8) {
        grs_bitmap decoded;
        gr_rsd8_convert(bm, &decoded);
        surface = SDL_CreateRGBSurfaceFrom(decoded.bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    } else {
        surface = SDL_CreateRGBSurfaceFrom(bm->bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    }
    SDL_SetSurfacePalette(surface, sdlPalette);

    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);

    SDL_FreeSurface(rgba);
    SDL_FreeSurface(surface);
}

void opengl_cache_texture(int idx, int size, grs_bitmap *bm) {
    SDL_GL_MakeCurrent(window, context);

    if (idx < NUM_LOADED_TEXTURES) {
        // only load the full-resolution into GL; use it in place of
        // down-scaled versions.
        if (size == TEXTURE_128_INDEX)
            convert_texture(textures[idx], bm);
        texturesByBitsPtr[bm->bits] = textures[idx];
    }
}

static void set_texture(grs_bitmap *bm) {
    std::map<uint8_t *, GLuint>::iterator iter = texturesByBitsPtr.find(bm->bits);
    if (iter != texturesByBitsPtr.end()) {
        glBindTexture(GL_TEXTURE_2D, iter->second);
    } else {
        convert_texture(dynTexture, bm);
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

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, ViewMatrix);

    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, ProjectionMatrix);

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _blend_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _blend_mode);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(shaderProgram, "light");

    glBegin(GL_TRIANGLE_STRIP);
    draw_vertex(*(vp[1]), tcAttrib, lightAttrib);
    draw_vertex(*(vp[0]), tcAttrib, lightAttrib);
    draw_vertex(*(vp[2]), tcAttrib, lightAttrib);
    if (n > 3)
        draw_vertex(*(vp[3]), tcAttrib, lightAttrib);
    glEnd();

    return CLIP_NONE;
}

float convx(float x) {
    return  x/32768.0f / render_width - 1;
}

float convy(float y) {
    return -y/32768.0f / render_height + 1;
}

int opengl_bitmap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_tmap_info *ti) {
    if (n != 4) {
        WARN("Unexpected number of bitmap vertices (%d)", n);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, IdentityMatrix);

    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, IdentityMatrix);

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(shaderProgram, "light");

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _blend_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _blend_mode);

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

void set_color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
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

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, ViewMatrix);

    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _blend_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _blend_mode);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(shaderProgram, "light");

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

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, ViewMatrix);

    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
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

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(shaderProgram, "light");

    g3s_point& vertex = *(p[0]);

    set_color(200, 200, 200, 255);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _blend_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _blend_mode);

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

    // Make sure everything starts with a stencil of 0
    glClearStencil(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);

    // Draw everything with a stencil of 0
    opengl_set_stencil(0x00);
}

#endif // USE_OPENGL
