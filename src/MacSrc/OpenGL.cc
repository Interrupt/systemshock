#include "OpenGL.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "mainloop.h"

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
    "\n"
    "varying vec2 TexCoords;\n"
    "\n"
    "uniform mat4 view;\n"
    "uniform mat4 proj;\n"
    "\n"
    "void main() {\n"
    "    TexCoords = texcoords;\n"
    "    gl_Position = proj * view * gl_Vertex;\n"
    "}\n";

static const char *FragmentShader =
    "#version 110\n"
    "\n"
    "varying vec2 TexCoords;\n"
    "\n"
    "uniform sampler2D tex;\n"
    "\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex, TexCoords);\n"
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

    glEnable(GL_DEPTH_TEST);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FragmentShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    auto view = glm::lookAt(
        glm::vec3(0.0f, 0.0f,  0.01f),
        glm::vec3(0.0f, 0.0f, -100.0f),
        glm::vec3(0.0f, 1.0f,  0.0f)
    );
    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, false, glm::value_ptr(view));

    auto proj = glm::perspective(glm::radians(89.5f), 1.0f, 0.1f, 100.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, false, glm::value_ptr(proj));

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
    return _use_opengl && _current_loop == FULLSCREEN_LOOP;
}

void toggle_opengl() {
    _use_opengl = !_use_opengl;
}

int opengl_draw_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) {
    return g3_draw_tmap(n, vp, bm);
}

int opengl_light_tmap(int n, g3s_phandle *vp, grs_bitmap *bm) {
    SDL_GL_MakeCurrent(window, context);

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(bm->bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    SDL_SetSurfacePalette(surface, sdlPalette);
    SDL_Surface *texture = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB24, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bm->w, bm->h, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->pixels);

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");

    struct g3s_point *p = *vp;
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0);
    glVertexAttrib2f(tcAttrib, 0, 0);
    glVertex3f(p[0].x / 65536.0f,  p[0].y / 65536.0f, -p[0].z / 65536.0f);
    glTexCoord2f(1, 0);
    glVertexAttrib2f(tcAttrib, 1, 0);
    glVertex3f(p[1].x / 65536.0f,  p[1].y / 65536.0f, -p[1].z / 65536.0f);
    glTexCoord2f(0, 1);
    glVertexAttrib2f(tcAttrib, 0, 1);
    glVertex3f(p[3].x / 65536.0f,  p[3].y / 65536.0f, -p[3].z / 65536.0f);
    glTexCoord2f(1, 1);
    glVertexAttrib2f(tcAttrib, 1, 1);
    glVertex3f(p[2].x / 65536.0f,  p[2].y / 65536.0f, -p[2].z / 65536.0f);
    glEnd();

    SDL_FreeSurface(texture);
    SDL_FreeSurface(surface);
    return 0;
}
