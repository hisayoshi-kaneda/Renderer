cmake_minimum_required (VERSION 3.8)

set(EXT_DIR "${CMAKE_CURRENT_LIST_DIR}/ext")

set(SRC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "")
set(EXT_INCLUDE_DIRS
    "${EXT_DIR}"
    "${EXT_DIR}/glad"
	"${EXT_DIR}/imgui"
	"${EXT_DIR}/imgui/examples"
	"${EXT_DIR}/stb"
	"${EXT_DIR}/tinyply/source"
    CACHE PATH "")

file(GLOB SRC_FILES
    "*.h"
	"core/*.cpp"
	"core/*.h"
	"mesh/*.cpp"
	"mesh/*.h"
	"opengl-wrapper/*.cpp"
	"opengl-wrapper/*.h")

file(GLOB SRC_FILES_RELATIVE RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	${SRC_FILES})

file(GLOB EXT_FILES
     "ext/glad/*.cpp"
     "ext/glad/*.h"
     "ext/stb/stb_image.h"
     "ext/stb/stb_image_write.h"
     "ext/imgui/*.cpp"
     "ext/imgui/*.h"
     "ext/imgui/examples/imgui_impl_opengl3.cpp"
     "ext/imgui/examples/imgui_opengl3.h"
     "ext/imgui/examples/imgui_impl_glfw.cpp"
     "ext/imgui/examples/imgui_glfw.h"
     "ext/tinyply/source/tinyply.cpp"
     "ext/tinyply/source/tinyply.h")

file(GLOB EXT_FILES_RELATIVE RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/ext"
   ${EXT_FILES})

file(GLOB SHADER_FILES
	"shaders/*.vert"
	"shaders/*.frag")

include_directories(${SRC_INCLUDE_DIR} ${EXT_INCLUDE_DIRS} ${GLM_INCLUDE_DIR} ${GLFW3_INCLUDE_DIR})

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 
source_group("Shader Files" FILES ${SHADER_FILES})

foreach(ext IN LISTS EXT_FILES_RELATIVE)
	get_filename_component(ext_dir ${ext} PATH)
	string(REPLACE "/" "\\" ext_dir ${ext_dir})
    source_group("External Files\\${ext_dir}" FILES "ext/${ext}")
endforeach()

foreach(src IN LISTS SRC_FILES_RELATIVE)
	get_filename_component(src_dir "${src}" PATH)
    source_group("Source Files\\${src_dir}" FILES "${src}")
endforeach()

set(main_app "Renderer")

add_executable(${main_app} "main.cpp" ${SRC_FILES} ${EXT_FILES} ${SHADER_FILES})
target_link_libraries(${main_app} ${OPENGL_LIBRARIES} ${GLFW3_LIBRARY})