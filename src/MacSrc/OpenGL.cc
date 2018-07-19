#include "OpenGL.h"

#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include </usr/local/include/glm/glm.hpp>
#include </usr/local/include/glm/gtc/matrix_transform.hpp>
#include </usr/local/include/glm/gtc/type_ptr.hpp>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include "mainloop.h"
#include "frflags.h"
#include "Shock.h"

extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern SDL_Palette *sdlPalette;

bool _use_opengl = true;

static SDL_GLContext context;
static GLuint shaderProgram;
static GLuint vbo;
static GLuint ebo;
static GLuint tex;

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

static GLuint compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, nullptr, buffer);
        printf("Error compiling shader!\n");
        puts(buffer);
        exit(1);
    }
    return shader;
}

int init_opengl() {
    context = SDL_GL_CreateContext(window);

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FragmentShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
        int border_x = (width - logical_width * scale_y + 1);
        glViewport(border_x / 2, 0, width - border_x, height);
    } else {
        // physical aspect ratio is narrower; black borders at top and bottom
        int border_y = (height - logical_height * scale_x + 1);
        glViewport(0, border_y / 2, width, height - border_y);
    }
}

bool use_opengl() {
    extern uint _fr_curflags;
    return _use_opengl &&
           _current_loop == FULLSCREEN_LOOP &&
           !(_fr_curflags & (FR_PICKUPM_MASK | FR_HACKCAM_MASK));
}

bool should_opengl_swap() {
    return _use_opengl &&
           _current_loop == FULLSCREEN_LOOP;
}

void toggle_opengl() {
    _use_opengl = !_use_opengl;
}

static void set_texture(grs_bitmap *bm) {
    // This function is too slow! Need to find a way to do it once per
    // bitmap, not for each surface to be drawn.
    // TODO: Translucent bitmaps

    SDL_Surface *surface;
    if (bm->type == BMT_RSD8) {
        grs_bitmap decoded;
        gr_rsd8_convert(bm, &decoded);
        surface = SDL_CreateRGBSurfaceFrom(decoded.bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    } else {
        surface = SDL_CreateRGBSurfaceFrom(bm->bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    }
    SDL_SetSurfacePalette(surface, sdlPalette);
    SDL_Surface *texture = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm->w, bm->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->pixels);
    SDL_FreeSurface(texture);
    SDL_FreeSurface(surface);
}

static void draw_vertex(const g3s_point& vertex, grs_bitmap *bm, GLint tcAttrib, GLint lightAttrib) {
    glVertexAttrib2f(tcAttrib, vertex.uv.u / 256.0, vertex.uv.v / 256.0);
    glVertexAttrib1f(lightAttrib, 1.0f - vertex.i / 4096.0f);
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

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f,  0.01f),
        glm::vec3(0.0f, 0.0f, -100.0f),
        glm::vec3(0.0f, 1.0f,  0.0f)
    );
    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, glm::value_ptr(view));

    glm::mat4 proj = glm::perspective(glm::radians(89.5f), 1.0f, 0.1f, 100.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, glm::value_ptr(proj));

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(shaderProgram, "light");

    glBegin(GL_TRIANGLE_STRIP);
    draw_vertex(*(vp[1]), bm, tcAttrib, lightAttrib);
    draw_vertex(*(vp[0]), bm, tcAttrib, lightAttrib);
    draw_vertex(*(vp[2]), bm, tcAttrib, lightAttrib);
    if (n > 3)
        draw_vertex(*(vp[3]), bm, tcAttrib, lightAttrib);
    glEnd();

    return CLIP_NONE;
}

float convx(float x) {
    return  x/32768.0f / gScreenWide - 1;
}

float convy(float y) {
    return -y/32768.0f / gScreenHigh + 1;
}

int opengl_bitmap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_tmap_info *ti) {
    if (n != 4) {
        WARN("Unexpected number of bitmap vertices (%d)", n);
        return CLIP_ALL;
    }

    SDL_GL_MakeCurrent(window, context);

    glm::mat4 view(1.0f);
    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, glm::value_ptr(view));

    glm::mat4 proj(1.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, glm::value_ptr(proj));

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    GLint lightAttrib = glGetAttribLocation(shaderProgram, "light");

    set_texture(bm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
