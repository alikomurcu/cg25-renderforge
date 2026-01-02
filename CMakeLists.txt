cmake_minimum_required(VERSION 3.25)
project(frg LANGUAGES CXX)

# --- Build setup
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/external CACHE PATH "Missing description." FORCE)

# --- Assimp ---
FetchContent_Declare(assimp
GIT_REPOSITORY https://github.com/assimp/assimp.git
GIT_TAG master)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# --- stb_image ---
FetchContent_Declare(stb
GIT_REPOSITORY https://github.com/nothings/stb.git
GIT_TAG master)
FetchContent_MakeAvailable(stb)

# --- Vulkan SDK
find_package(Vulkan REQUIRED)

# --- glslc (from Vulkan SDK) ---
set(GLSLC_HINT_DIRS
    $ENV{VULKAN_SDK}/Bin
    $ENV{VULKAN_SDK}/bin
    $ENV{VULKAN_SDK}/macOS/bin
)
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS ${GLSLC_HINT_DIRS} REQUIRED)

# ---- GLFW ------------------------------------------------------------------
set(GLFW_ROOT "${CMAKE_SOURCE_DIR}/external/glfw")

set(GLFW_LINK_TARGET glfw3)

if (WIN32)
    # Static .lib shipped with the repo for Windows
    add_library(glfw3 STATIC IMPORTED GLOBAL)
    set_target_properties(glfw3 PROPERTIES
        IMPORTED_LOCATION "${GLFW_ROOT}/lib-vc2022/glfw3.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${GLFW_ROOT}/include"
    )
else()
    # Rely on system-provided GLFW (e.g. Homebrew on macOS)
    find_package(glfw3 REQUIRED)
    if (TARGET glfw)
        set(GLFW_LINK_TARGET glfw)
    elseif (TARGET glfw3)
        set(GLFW_LINK_TARGET glfw3)
    elseif (TARGET glfw::glfw)
        set(GLFW_LINK_TARGET glfw::glfw)
    else()
        message(FATAL_ERROR "glfw3 was found but no usable CMake target is available.")
    endif()
endif()

# On Windows, GLFW needs these system libs when linking statically
if (MSVC)
    set(GLFW_WIN_SYS_LIBS user32 gdi32 shell32 ole32 advapi32 winmm)
endif()

# ---- Executable -------------------------------------------------------------
add_executable(frg
    src/main.cpp
    src/first_app.cpp
    src/frg_window.cpp
    src/frg_pipeline.cpp
    src/frg_device.cpp
    src/frg_swap_chain.cpp
    src/frg_model.cpp
    src/frg_renderer.cpp
    src/simple_render_system.cpp
    src/frg_camera.cpp
    src/frg_mesh.cpp
    src/frg_descriptor.cpp
    src/frg_game_object.cpp
    src/keyboard_movement_controller.cpp
    src/frg_lighting.cpp
    src/frg_ssbo.cpp
)

# --- Shader compilation (GLSL -> SPIR-V) ---
file(GLOB SHADER_SOURCES "${CMAKE_SOURCE_DIR}/shaders/*.vert" "${CMAKE_SOURCE_DIR}/shaders/*.frag")
set(SPV_OUTPUTS)
foreach(shader ${SHADER_SOURCES})
    get_filename_component(fname "${shader}" NAME)
    set(out "${CMAKE_BINARY_DIR}/shaders/${fname}.spv")
    add_custom_command(
        OUTPUT "${out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/shaders"
        COMMAND ${GLSLC_EXECUTABLE} "${shader}" -o "${out}"
        DEPENDS "${shader}"
        COMMENT "Compiling GLSL ${fname} -> SPIR-V"
        VERBATIM
    )
    list(APPEND SPV_OUTPUTS "${out}")
endforeach()
add_custom_target(compile_shaders ALL DEPENDS ${SPV_OUTPUTS})
add_dependencies(frg compile_shaders)

# Copy compiled shaders next to the built executable after each build
add_custom_command(
    TARGET frg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:frg>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/shaders" "$<TARGET_FILE_DIR:frg>/shaders"
    COMMENT "Copying compiled shaders to the runtime output directory"
)

# In Visual Studio, run the app with the exe folder as working directory
if (MSVC)
    set_property(TARGET frg PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:frg>")
endif()

# (Optional) install rule so `cmake --install` bundles shaders with the exe
install(TARGETS frg RUNTIME DESTINATION .)
install(DIRECTORY "${SHADER_DIR}" DESTINATION .)




# Make the headers visible to frg
target_include_directories(frg PRIVATE
    "${CMAKE_SOURCE_DIR}/external/glm"
    "${CMAKE_SOURCE_DIR}/src"
    external/assimp-src/include
    external/stb-src
)

target_link_libraries(frg PRIVATE
    Vulkan::Vulkan
    ${GLFW_LINK_TARGET}
    ${GLFW_WIN_SYS_LIBS}
    assimp
)

if (APPLE)
    target_compile_definitions(frg PRIVATE VK_ENABLE_BETA_EXTENSIONS)
endif()
