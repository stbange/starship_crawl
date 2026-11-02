#include "gl_api.h"
GLApi gl;
bool gl_load(void) {
#ifdef __EMSCRIPTEN__
#define SC_GL_LOAD(ret, name, args) gl.name = gl##name;
#else
#define SC_GL_LOAD(ret, name, args)                                                                \
    gl.name = (ret(APIENTRY *) args)SDL_GL_GetProcAddress("gl" #name);                             \
    if (!gl.name) {                                                                                \
        SDL_Log("Missing OpenGL function: gl%s", #name);                                           \
        return false;                                                                              \
    }
#endif
    SC_GL_FUNCTIONS(SC_GL_LOAD)
#undef SC_GL_LOAD
    return true;
}
