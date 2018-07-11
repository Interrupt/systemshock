#include "OpenGL.h"

#include <GL/glew.h>
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

static const char *VertexShader = R"glsl(
    #version 150 core

    in vec3 position;
    in vec2 texcoords;

    out vec2 TexCoords;

    uniform mat4 view;
    uniform mat4 proj;

    void main() {
        TexCoords = texcoords;
        gl_Position = proj * view * vec4(position, 1.0);
    }
)glsl";

static const char *FragmentShader = R"glsl(
    #version 150 core

    in vec2 TexCoords;

    out vec4 outColor;

    uniform sampler2D tex;

    void main() {
        outColor = texture(tex, TexCoords);
    }
)glsl";

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

    glewInit();

    glEnable(GL_DEPTH_TEST);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, VertexShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, FragmentShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glGenTextures(1, &tex);

    const GLuint elements[] = {
        0, 1, 3,
        1, 2, 3
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

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

    uint8_t pixels[64*64*3];
    uint8_t *p = pixels;
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 64; ++j) {
            *p++ = 4 * i;
            *p++ = 4 * j;
            *p++ = 0;
        }
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

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

    struct g3s_point *p = *vp;
    const float vertices[] = {
        p[0].x / 65536.0f,  p[0].y / 65536.0f, -p[0].z / 65536.0f, 0.0, 0.0,
        p[1].x / 65536.0f,  p[1].y / 65536.0f, -p[1].z / 65536.0f, 1.0, 0.0,
        p[2].x / 65536.0f,  p[2].y / 65536.0f, -p[2].z / 65536.0f, 1.0, 1.0,
        p[3].x / 65536.0f,  p[3].y / 65536.0f, -p[3].z / 65536.0f, 0.0, 1.0,
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

    GLint tcAttrib = glGetAttribLocation(shaderProgram, "texcoords");
    glEnableVertexAttribArray(tcAttrib);
    glVertexAttribPointer(tcAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(bm->bits, bm->w, bm->h, 8, bm->row, 0, 0, 0, 0);
    SDL_SetSurfacePalette(surface, sdlPalette);
    SDL_Surface *texture = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB24, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bm->w, bm->h, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->pixels);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SDL_FreeSurface(texture);
    SDL_FreeSurface(surface);
    return 0;
}
