#ifndef __amorphous_Wasm__
#define __amorphous_Wasm__


#include <emscripten.h>
#include <string>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
extern "C" {
    #include "emscripten/html5.h"
}


class WasmApp {

public:

    virtual ~WasmApp() {}

    virtual int Init() { return 0; }

    virtual void Render() = 0;
};


class Wasm {

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context;

public:

    Wasm() : webgl_context(0) {
    }

    ~Wasm();

    int Init(const char *target_canvas_element_id, unsigned int width, unsigned int height);

    void Render();
};


// global functions
void console_log(const std::string& text);


#endif /* __amorphous_Wasm__ */
