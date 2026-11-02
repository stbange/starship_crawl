#ifndef SC_GL_API_H
#define SC_GL_API_H
#include <SDL3/SDL.h>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_opengl_glext.h>
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
#define SC_GL_FUNCTIONS(X)                                                                         \
    X(void, Viewport, (GLint, GLint, GLsizei, GLsizei))                                            \
    X(void, Scissor, (GLint, GLint, GLsizei, GLsizei))                                             \
    X(void, ClearColor, (GLfloat, GLfloat, GLfloat, GLfloat))                                      \
    X(void, Clear, (GLbitfield))                                                                   \
    X(void, Enable, (GLenum))                                                                      \
    X(void, Disable, (GLenum))                                                                     \
    X(void, DepthFunc, (GLenum))                                                                   \
    X(void, GenBuffers, (GLsizei, GLuint *)) X(void, BindBuffer, (GLenum, GLuint)) X(              \
        void, BufferData, (GLenum, GLsizeiptr, const void *, GLenum)) X(void, DeleteBuffers,       \
                                                                        (GLsizei, const GLuint *)) \
        X(void, GenVertexArrays, (GLsizei, GLuint *)) X(void, BindVertexArray, (GLuint)) X(        \
            void, DeleteVertexArrays, (GLsizei, const GLuint *)) X(void, EnableVertexAttribArray,  \
                                                                   (GLuint))                       \
            X(void, VertexAttribPointer,                                                           \
              (GLuint, GLint, GLenum, GLboolean, GLsizei, const void *)) X(GLuint, CreateShader,   \
                                                                           (GLenum))               \
                X(void, ShaderSource, (GLuint, GLsizei, const GLchar *const *, const GLint *)) X(  \
                    void,                                                                          \
                    CompileShader, (GLuint)) X(void, GetShaderiv, (GLuint, GLenum, GLint *))       \
                    X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei *, GLchar *)) X(           \
                        void,                                                                      \
                        DeleteShader,                                                              \
                        (                                                                          \
                            GLuint)) X(GLuint, CreateProgram,                                      \
                                       (void)) X(void, AttachShader, (GLuint, GLuint))             \
                        X(void, LinkProgram, (GLuint)) X(                                          \
                            void,                                                                  \
                            GetProgramiv,                                                          \
                            (GLuint, GLenum,                                                       \
                             GLint *)) X(void, GetProgramInfoLog,                                  \
                                         (GLuint, GLsizei, GLsizei *, GLchar *))                   \
                            X(void, UseProgram, (GLuint)) X(void, DeleteProgram, (GLuint)) X(      \
                                GLint, GetUniformLocation,                                         \
                                (GLuint,                                                           \
                                 const GLchar                                                      \
                                     *)) X(void, UniformMatrix4fv,                                 \
                                           (GLint, GLsizei, GLboolean, const GLfloat *))           \
                                X(void, Uniform3f, (GLint, GLfloat, GLfloat, GLfloat)) X(          \
                                    void,                                                          \
                                    Uniform1f,                                                     \
                                    (GLint, GLfloat)) X(void, Uniform1i, (GLint, GLint))           \
                                    X(void, DrawArrays, (GLenum, GLint, GLsizei)) X(               \
                                        void,                                                      \
                                        GenFramebuffers,                                           \
                                        (GLsizei,                                                  \
                                         GLuint *)) X(void, BindFramebuffer, (GLenum, GLuint))     \
                                        X(void, DeleteFramebuffers, (GLsizei, const GLuint *)) X(  \
                                            void,                                                  \
                                            FramebufferTexture2D,                                  \
                                            (GLenum, GLenum, GLenum, GLuint, GLint))               \
                                            X(GLenum, CheckFramebufferStatus,                      \
                                              (GLenum)) X(void, GenTextures, (GLsizei, GLuint *))  \
                                                X(void, BindTexture, (GLenum, GLuint)) X(          \
                                                    void,                                          \
                                                    TexImage2D,                                    \
                                                    (GLenum, GLint, GLint, GLsizei, GLsizei,       \
                                                     GLint, GLenum, GLenum, const void *))         \
                                                    X(void, TexParameteri,                         \
                                                      (GLenum, GLenum,                             \
                                                       GLint)) X(void, DeleteTextures,             \
                                                                 (GLsizei, const GLuint *))        \
                                                        X(void, GenRenderbuffers,                  \
                                                          (                                        \
                                                              GLsizei,                             \
                                                              GLuint *)) X(void, BindRenderbuffer, \
                                                                           (GLenum, GLuint))       \
                                                            X(void, RenderbufferStorage,           \
                                                              (GLenum, GLenum, GLsizei, GLsizei))  \
                                                                X(void, FramebufferRenderbuffer,   \
                                                                  (GLenum, GLenum, GLenum,         \
                                                                   GLuint))                        \
                                                                    X(void, DeleteRenderbuffers,   \
                                                                      (GLsizei, const GLuint *))   \
                                                                        X(void, BlitFramebuffer,   \
                                                                          (GLint, GLint, GLint,    \
                                                                           GLint, GLint, GLint,    \
                                                                           GLint, GLint,           \
                                                                           GLbitfield, GLenum))    \
                                                                            X(void, ReadPixels,    \
                                                                              (GLint, GLint,       \
                                                                               GLsizei, GLsizei,   \
                                                                               GLenum, GLenum,     \
                                                                               void                \
                                                                                   *)) X(GLenum,   \
                                                                                         GetError, \
                                                                                         (void))   \
                                                                                X(const GLubyte *, \
                                                                                  GetString,       \
                                                                                  (GLenum))
typedef struct {
#define SC_GL_FIELD(ret, name, args) ret(APIENTRY *name) args;
    SC_GL_FUNCTIONS(SC_GL_FIELD)
#undef SC_GL_FIELD
} GLApi;
extern GLApi gl;
bool gl_load(void);
#endif
