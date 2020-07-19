#include "core/Timer.h"
#include "core/common.h"
#include "PathTracer.h"


int main(int argc, char **argv) {
    Shader::shadersDir = filesystem::path(argv[0]).parent_path() / "shaders";
    shared_ptr<Window> window = make_shared<Window>(640, 480, "window");
    PathTracer pt(window);
    pt.render();
}

