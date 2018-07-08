#include "Wasm.hpp"


// To be implemented by the user.
WasmApp *CreateWasmAppInstance();


Wasm wasm;


WasmApp *wasm_app_instance = nullptr;


Wasm::~Wasm() {
    emscripten_webgl_destroy_context(webgl_context);
}

int Wasm::Init(const char *target_canvas_element_id, unsigned int width, unsigned int height) {

    wasm_app_instance = CreateWasmAppInstance();

    if(!wasm_app_instance) {
        console_log("CreateWasmAppInstance returned null.");
        return -1;
    }

    // Context configurations
    EmscriptenWebGLContextAttributes attrs;
    attrs.explicitSwapControl = 0;
    attrs.depth = 1;
    attrs.stencil = 1;
    attrs.antialias = 1;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;

    webgl_context = emscripten_webgl_create_context(target_canvas_element_id, &attrs);
    if(webgl_context < 0)
        console_log("Failed to create a WebGL context.\n");

    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(webgl_context);

    if(res < 0) {
        console_log("!!!ewmcc");
        return -1;
    }

    if(wasm_app_instance)
        return wasm_app_instance->Init();
    else {
        console_log("!!!wasm_app_instance");
        return -1;
    }
}

void Wasm::Render() {

    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(webgl_context);

    if(res != EMSCRIPTEN_RESULT_SUCCESS) {
        console_log("WebGL context error\n");
    }

    if(wasm_app_instance)
        wasm_app_instance->Render();
    else {
        console_log("!wasm_app_instance");
    }
}


void console_log(const std::string& text) {

    EM_ASM_({
        console.log("wasm:",Pointer_stringify($0));
    }, text.c_str());
}


extern "C" {

    /**
     * \brief Called from JavaScript code once all the compiled functions are ready.
     */
    int wasm_init() {
        console_log("wasm_init");
        return wasm.Init("mycanvas",1280,720);
    }


    void wasm_render() {
        wasm.Render();
    }

}
