include(FindPackageHandleStandardArgs)

set(GLM_DIR "" CACHE PATH "")

find_path(GLM_INCLUDE_DIR
          NAMES glm/glm.hpp
          PATHS
          /usr/include
          /usr/local/include
          C:/Libraries/glm/include
          ${GLM_DIR}/include)

find_package_handle_standard_args(
    GLM
    DEFAULT_MSG
    GLM_INCLUDE_DIR
)

mark_as_advanced(GLM_DIR GLM_INCLUDE_DIR GLM_LIBRARY)

if (GLM_FOUND)
    message(STATUS "GLM include: ${GLM_INCLUDE_DIR}")
    set(GLM_INCLUDE_DIRS "${GLM_INCLUDE_DIR}")
endif()
