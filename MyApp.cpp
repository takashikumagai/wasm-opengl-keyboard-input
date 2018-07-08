#include "MyApp.hpp"
#include <vector>
#include <cmath>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
extern "C" {
    #include "emscripten/html5.h"
}


WasmApp *CreateWasmAppInstance() {
    return new MyApp;
}


static GLuint program = 0;

static const char *vertex_shader_source =
    "#version 300 es\n"\
    "layout(location=0) in vec4 pos;\n"\
    "layout(location=1) in vec3 n;\n"\
    "out vec3 normal;\n"\
    "uniform mat4 W;"\
    "uniform mat4 V;"\
    "uniform mat4 P;"\
    "uniform mat4 Rx;"\
    "uniform mat4 Ry;"\
    "void main() {gl_Position = P*V*W*Ry*Rx*pos;normal=mat3(W*Ry*Rx)*n;}";

static const char *fragment_shader_source =
    "#version 300 es\n"\
    "precision mediump float;\n"\
    "in vec3 normal;\n"\
    "layout(location=0) out vec4 fc;\n"\
    "uniform vec3 dir_to_light;\n"\
    "void main() {"\
        "vec3 n = normalize(normal);"\
        "float d = dot(dir_to_light,n);"\
        "float f = (d+1.0)*0.5;"\
        "vec3 c = f*vec3(1.0,1.0,1.0) + (1.0-f)*vec3(0.2,0.2,0.2);"\
        "fc = vec4(c.x,c.y,c.z,1.0);"\
    "}";

static const GLfloat cube_vertices[] = {
    // top
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f,  1.0f,
     1.0f, 1.0f,  1.0f,
     1.0f, 1.0f, -1.0f,

    // bottom
     1.0f,-1.0f, -1.0f,
     1.0f,-1.0f,  1.0f,
    -1.0f,-1.0f,  1.0f,
    -1.0f,-1.0f, -1.0f,

    // right
     1.0f, 1.0f,  1.0f,
     1.0f,-1.0f,  1.0f,
     1.0f,-1.0f, -1.0f,
     1.0f, 1.0f, -1.0f,

    // left
    -1.0f, 1.0f, -1.0f,
    -1.0f,-1.0f, -1.0f,
    -1.0f,-1.0f,  1.0f,
    -1.0f, 1.0f,  1.0f,

    // near side
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,

    // far side
     1.0f, 1.0f, -1.0f,
     1.0f,-1.0f, -1.0f,
    -1.0f,-1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f
};

static const GLfloat cube_normals[] = {
    // top
     0.0f, 1.0f, 0.0f,
     0.0f, 1.0f, 0.0f,
     0.0f, 1.0f, 0.0f,
     0.0f, 1.0f, 0.0f,

     0.0f,-1.0f, 0.0f,
     0.0f,-1.0f, 0.0f,
     0.0f,-1.0f, 0.0f,
     0.0f,-1.0f, 0.0f,

     1.0f, 0.0f, 0.0f,
     1.0f, 0.0f, 0.0f,
     1.0f, 0.0f, 0.0f,
     1.0f, 0.0f, 0.0f,

    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,

     0.0f, 0.0f, 1.0f,
     0.0f, 0.0f, 1.0f,
     0.0f, 0.0f, 1.0f,
     0.0f, 0.0f, 1.0f,

     0.0f, 0.0f,-1.0f,
     0.0f, 0.0f,-1.0f,
     0.0f, 0.0f,-1.0f,
     0.0f, 0.0f,-1.0f
};

static const GLuint cube_indices[] = {
    0,1,2, 0,2,3,
    4,5,6, 4,6,7,
    8,9,10, 8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23
};

static GLuint vbo = 0;
static GLuint nbo = 0;
static GLuint ibo = 0;

float forward = 0;
float side = 0;
float camera_x = 0;
float camera_z = 0;

static int CheckShaderStatus(GLuint shader) {

    GLint is_compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

    if(is_compiled == GL_TRUE) {
        return 0;
    } else {
        // Shader compilation failed.
        GLint log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

        // log_size == the long length INCLUDING the terminating null character
        std::vector<GLchar> error_log(log_length);
        glGetShaderInfoLog(shader, log_length, &log_length, &error_log[0]);

        printf("gl shader error: %s",&error_log[0]);

        glDeleteShader(shader);

        return -1;
    }
}

static int CheckProgramStatus(GLuint program) {

    GLint is_linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, (int *)&is_linked);

    if(is_linked == GL_TRUE) {
        return 0;
    } else {
        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        std::vector<GLchar> error_log(log_length);
        glGetProgramInfoLog(program, log_length, &log_length, &error_log[0]);

        printf("gl program error: %s",&error_log[0]);

        glDeleteProgram(program);

        return -1;
    }
}

const char *emscripten_result_to_string(EMSCRIPTEN_RESULT result) {
  if (result == EMSCRIPTEN_RESULT_SUCCESS) return "EMSCRIPTEN_RESULT_SUCCESS";
  if (result == EMSCRIPTEN_RESULT_DEFERRED) return "EMSCRIPTEN_RESULT_DEFERRED";
  if (result == EMSCRIPTEN_RESULT_NOT_SUPPORTED) return "EMSCRIPTEN_RESULT_NOT_SUPPORTED";
  if (result == EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED) return "EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED";
  if (result == EMSCRIPTEN_RESULT_INVALID_TARGET) return "EMSCRIPTEN_RESULT_INVALID_TARGET";
  if (result == EMSCRIPTEN_RESULT_UNKNOWN_TARGET) return "EMSCRIPTEN_RESULT_UNKNOWN_TARGET";
  if (result == EMSCRIPTEN_RESULT_INVALID_PARAM) return "EMSCRIPTEN_RESULT_INVALID_PARAM";
  if (result == EMSCRIPTEN_RESULT_FAILED) return "EMSCRIPTEN_RESULT_FAILED";
  if (result == EMSCRIPTEN_RESULT_NO_DATA) return "EMSCRIPTEN_RESULT_NO_DATA";
  return "Unknown EMSCRIPTEN_RESULT!";
}

#define TEST_RESULT(x) if (ret != EMSCRIPTEN_RESULT_SUCCESS) printf("%s returned %s.\n", #x, emscripten_result_to_string(ret));

static inline const char *emscripten_event_type_to_string(int eventType) {
    const char *events[] = { "(invalid)", "(none)", "keypress", "keydown", "keyup", "click", "mousedown", "mouseup", "dblclick", "mousemove", "wheel", "resize", 
        "scroll", "blur", "focus", "focusin", "focusout", "deviceorientation", "devicemotion", "orientationchange", "fullscreenchange", "pointerlockchange", 
        "visibilitychange", "touchstart", "touchend", "touchmove", "touchcancel", "gamepadconnected", "gamepaddisconnected", "beforeunload", 
        "batterychargingchange", "batterylevelchange", "webglcontextlost", "webglcontextrestored", "mouseenter", "mouseleave", "mouseover", "mouseout", "(invalid)" };
    ++eventType;

    if(eventType < 0)
        eventType = 0;
    if(eventType >= sizeof(events)/sizeof(events[0]))
        eventType = sizeof(events)/sizeof(events[0])-1;

    return events[eventType];
}


// The event handler functions can return 1 to suppress the event and disable the default action. That calls event.preventDefault();
// Returning 0 signals that the event was not consumed by the code, and will allow the event to pass on and bubble up normally.
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData)
{
    char s[256];
    memset(s,0,sizeof(s));
    sprintf(s,"%s, key: \"%s\", code: \"%s\", location: %lu,%s%s%s%s repeat: %d, locale: \"%s\", char: \"%s\", charCode: %lu, keyCode: %lu, which: %lu\n",
    emscripten_event_type_to_string(eventType), e->key, e->code, e->location, 
    e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "", 
    e->repeat, e->locale, e->charValue, e->charCode, e->keyCode, e->which);
    console_log(std::string(s));

    std::string key = e->key;
    if(eventType == EMSCRIPTEN_EVENT_KEYPRESS ) {
        if((key == "f") || (e->which == 102)) { // &#102; = HTML-code for f
        }
        else if((key == "w") || (e->which == 119)) {
            forward = 1;
            return 1;
        }
        else if((key == "s") || (e->which == 115)) {
            forward = -1;
            return 1;
        }
        else if((key == "d") || (e->which == 0)) {
            side = 1;
            return 1;
        }
        else if((key == "a") || (e->which == 0)) {
            side = -1;
            return 1;
        }
    }
    else if(eventType == EMSCRIPTEN_EVENT_KEYUP ) {
        if((key == "w") || (e->which == 119) || (key == "s") || (e->which == 115)) {
            forward = 0;
            return 1;
        }
        else if((key == "d") || (e->which == 115)) {
            side = 0;
            return 1;
        }        
    }

    return 0;
}

MyApp::MyApp():
world_transform(0), view_transform(0) {
}

int MyApp::Init() {

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    if(vertex_shader == 0) {
        return -1;
    }

    GLint len = strlen(vertex_shader_source);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, &len);

    glCompileShader(vertex_shader);

    if( CheckShaderStatus(vertex_shader) < 0 ) {
        return -1;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    if(fragment_shader == 0) {
        return -1;
    }

    len = strlen(fragment_shader_source);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, &len);

    glCompileShader(fragment_shader);

    if( CheckShaderStatus(fragment_shader) < 0 ) {
        glDeleteShader(vertex_shader);
        return -1;
    }

    program = glCreateProgram();

    glAttachShader(program,vertex_shader);
    glAttachShader(program,fragment_shader);

    glLinkProgram(program);

    if( CheckProgramStatus(program) < 0 ) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return -1;
    }

    float proj[16] = {
        1.81f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 2.41f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.001f, 1.0f, // column 3
        0.0f, 0.0f, -0.1001f, 0.0f  // column 4
    };

    world_transform      = glGetUniformLocation(program,"W");
    view_transform       = glGetUniformLocation(program,"V");
    GLint projection_transform = glGetUniformLocation(program,"P");
    console_log(std::string("W: ") + std::to_string(world_transform));
    console_log(std::string("V: ") + std::to_string(view_transform));
    console_log(std::string("P: ") + std::to_string(projection_transform));
    glUseProgram(program);
    glUniformMatrix4fv(projection_transform, 1, GL_FALSE, proj);

    // Set the light direction vector
    float fv[] = {0.262705f, 0.938233f, 0.225176f};
    GLint dir_to_light = glGetUniformLocation(program,"dir_to_light");
    glUniform3fv(dir_to_light, 1, fv);

    // Set vertices - for each element we call 'Generate', 'Bind', and 'Buffer' APIs.
    glGenBuffers( 1, &vbo );
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glGenBuffers( 1, &nbo );
    glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);

    glGenBuffers( 1, &ibo );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    this->start_time = std::chrono::system_clock::now();
    this->prev_time = std::chrono::system_clock::now();

    EMSCRIPTEN_RESULT ret = emscripten_set_keypress_callback(0, 0, 1, key_callback);
    TEST_RESULT(emscripten_set_keypress_callback);
    ret = emscripten_set_keydown_callback(0, 0, 1, key_callback);
    TEST_RESULT(emscripten_set_keydown_callback);
    ret = emscripten_set_keyup_callback(0, 0, 1, key_callback);
    TEST_RESULT(emscripten_set_keyup_callback);

    return 0;
}

void MyApp::Render() {

    {
        static int count = 0;
        if(count < 5) {
            console_log("MyApp::Render");
            count += 1;
        }
    }
    //printf("");

    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> t = now - this->start_time;

	const float PI = 3.141593;
    float a = (float)fmod(t.count(),PI*2.0);
    float c = cosf(a);
    float s = sinf(a);
    float rx[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, c,    s,    0.0f, // column 2
        0.0f, -s,   c,    0.0f, // column 3
        0.0f, 0.0f, 0.0f, 1.0f  // column 4
    };

    a = (float)fmod(t.count()*0.9,PI*2.0);
    c = cosf(a);
    s = sinf(a);
    float ry[16] = {
        c,    0.0f, -s,   0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s,    0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    GLint rotation_x = glGetUniformLocation(program,"Rx");
    GLint rotation_y = glGetUniformLocation(program,"Ry");
    glUniformMatrix4fv(rotation_x, 1, GL_FALSE, rx);
    glUniformMatrix4fv(rotation_y, 1, GL_FALSE, ry);

    //unsigned int viewport_width = 1280;
    //unsigned int viewport_height = 720;
    //glViewport(0, 0, viewport_width, viewport_height);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    UpdateViewMatrix();

    // Draws the cube model by calling OpenGL APIs
    // glBindBuffer(): bind a named buffer object.
    // glEnableVertexAttribArray(): enable a generic vertex attribute array.
    // glVertexAttribPointer(): define an array of generic vertex attribute data.

    GLuint vertex_attrib_index = 0;
    GLuint normal_attrib_index = 1;
    GLuint texture_coord_attrib_index = 2;

    GLboolean normalized = GL_FALSE;

//  glBindBuffer( GL_ARRAY_BUFFER, 0 );
//  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );

    glEnableVertexAttribArray(vertex_attrib_index);
    glVertexAttribPointer(vertex_attrib_index, 3, GL_FLOAT, normalized, sizeof(float)*3, 0);

    // Drawing directly from a generic array - this does not work with emcc;
    //glVertexAttribPointer(vertex_attrib_index, 3, GL_FLOAT, normalized, stride, cube_vertices );

    glEnableVertexAttribArray(normal_attrib_index);
    glBindBuffer(GL_ARRAY_BUFFER, nbo);
    glVertexAttribPointer(normal_attrib_index, 3, GL_FLOAT, normalized, sizeof(float)*3, 0);

    RenderCubes();

    glDisableVertexAttribArray(vertex_attrib_index);
}

void MyApp::UpdateViewMatrix() {

    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> t = now - this->prev_time;
    this->prev_time = now;

    float dt = t.count();

    camera_x = camera_x + forward * dt * 4.0f;
    camera_z = camera_z + side    * dt * 4.0f;

    float view[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 1.0f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.0f, 0.0f, // column 3
        camera_x, 0.0f, camera_z + 5.0f, 1.0f  // column 4
    };

    glUniformMatrix4fv(this->view_transform, 1, GL_FALSE, view);
}

void MyApp::RenderCube(float tx, float ty, float tz) {

    float m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, // column 1
        0.0f, 1.0f, 0.0f, 0.0f, // column 2
        0.0f, 0.0f, 1.0f, 0.0f, // column 3
        tx,   ty,   tz,   1.0f  // column 4
    };

    glUniformMatrix4fv(world_transform, 1, GL_FALSE, m);

    GLsizei num_elements_to_render = 36;
    //glDrawElements( GL_TRIANGLES, num_elements_to_render, GL_UNSIGNED_INT, cube_indices );
    glDrawElements( GL_TRIANGLES, num_elements_to_render, GL_UNSIGNED_INT, 0 );
}

void MyApp::RenderCubes() {

    for(int y=0; y<2; y++) {
        float ty = (float)y*6.0f - 3.0f;
        for(int z=0; z<10; z++) {
            float tz = (float)z*4.0f - 20.0f + 15.0f;
            for(int x=0; x<10; x++) {
                float tx = (float)x*4.0f - 20.0f;
                RenderCube(tx,ty,tz);
            }
        }
    }
}
