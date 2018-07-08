emcc -s WASM=1 -s USE_WEBGL2=1 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']" -s "EXPORTED_FUNCTIONS=[_wasm_init, _wasm_render]" -std=c++1z -I~/src/emsdk/emscripten/1.38.0/system/include Wasm.cpp MyApp.cpp

