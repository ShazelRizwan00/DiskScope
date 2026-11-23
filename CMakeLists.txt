cmake_minimum_required(VERSION 3.24)

project(DiskScope
    VERSION 1.0.0
    DESCRIPTION "DiskScope - disk space analyzer with a hand-written max binary heap"
    LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# Third-party dependencies (pinned, reproducible)
#   Dear ImGui   v1.91.9b
#   ImPlot       v0.17
#   GLFW         3.4      (built from source)
# ImGui ships its own OpenGL 3 loader (imgui_impl_opengl3_loader.h), so no
# GLAD/GLEW/GLEW dependency is required.
# ---------------------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    URL https://codeload.github.com/glfw/glfw/tar.gz/refs/tags/3.4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.91.9b
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_Declare(implot
    URL https://codeload.github.com/epezent/implot/tar.gz/refs/tags/v0.17
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(glfw imgui implot)

# Dear ImGui: core + GLFW/OpenGL3 backends (no CMakeLists upstream, so we build it)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
if(MSVC)
    target_compile_options(imgui PRIVATE /W3)
else()
    target_compile_options(imgui PRIVATE -w)
endif()

# ImPlot: plotting library used by the analytics charts
add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp)
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)
if(MSVC)
    target_compile_options(implot PRIVATE /W3)
else()
    target_compile_options(implot PRIVATE -w)
endif()

# ---------------------------------------------------------------------------
# diskscope_core - DSA + filesystem engine, deliberately free of any GUI deps
# ---------------------------------------------------------------------------
add_library(diskscope_core STATIC
    src/core/FileSystemScanner.cpp
    src/core/SizeCalculator.cpp
    src/structures/MaxHeap.cpp
    src/services/AnalysisEngine.cpp
    src/services/ScanController.cpp
    src/services/HeapDemoEngine.cpp
    src/utils/Formatting.cpp
    src/platform/AllocatedSize.cpp
    src/platform/FileAttributes.cpp)
target_include_directories(diskscope_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(diskscope_core PUBLIC
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
    $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601>)

# ---------------------------------------------------------------------------
# diskscope_ui - application shell, views and visualizations
# ---------------------------------------------------------------------------
add_library(diskscope_ui STATIC
    src/ui/Application.cpp
    src/ui/MainWindow.cpp
    src/ui/Theme.cpp
    src/ui/Dashboard.cpp
    src/ui/DirectoryExplorer.cpp
    src/ui/LargestFoldersView.cpp
    src/ui/HeapVisualizerView.cpp
    src/ui/SettingsView.cpp
    src/visualization/HeapRenderer.cpp
    src/visualization/ChartRenderer.cpp
    src/platform/NativeFolderDialog.cpp)
target_link_libraries(diskscope_ui PUBLIC diskscope_core imgui implot)

if(WIN32)
    # IFileDialog (folder picker) + shell helpers
    target_link_libraries(diskscope_ui PUBLIC ole32 uuid shell32)
endif()

# ---------------------------------------------------------------------------
# DiskScope executable
# ---------------------------------------------------------------------------
add_executable(DiskScope src/main.cpp)
target_link_libraries(DiskScope PRIVATE diskscope_ui)

# ---------------------------------------------------------------------------
# Tests (lightweight custom harness, registered with CTest)
# ---------------------------------------------------------------------------
enable_testing()

add_executable(diskscope_tests
    tests/test_main.cpp
    tests/test_heap.cpp
    tests/test_scanner.cpp
    tests/test_analysis.cpp)
target_link_libraries(diskscope_tests PRIVATE diskscope_core)
target_include_directories(diskscope_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_test(NAME diskscope_tests COMMAND diskscope_tests)
set_tests_properties(diskscope_tests PROPERTIES TIMEOUT 900)
