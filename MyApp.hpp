#ifndef __amorphous_MyApp__
#define __amorphous_MyApp__

#include "Wasm.hpp"
#include <chrono>


class MyApp : public WasmApp {

    std::chrono::time_point<std::chrono::system_clock> start_time;

    std::chrono::time_point<std::chrono::system_clock> prev_time;

    GLint world_transform;
    GLint view_transform;

    void RenderCube(float tx, float ty, float tz);

    void RenderCubes();

    void UpdateViewMatrix();

public:

    MyApp();

    int Init();

    void Render();
};

#endif /* __amorphous_MyApp__ */
