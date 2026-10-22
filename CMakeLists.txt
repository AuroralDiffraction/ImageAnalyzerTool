cmake_minimum_required(VERSION 3.19)

project(ImageAnalyzerTool
    VERSION 0.2.0
    DESCRIPTION "Dark-frame calibration and noise analysis for image stacks (MATLAB .mat, TIFF, PNG, JPEG)"
    HOMEPAGE_URL "https://github.com/AuroralDiffraction"
    LANGUAGES CXX)

# Shown in Help > About, the status bar and the .exe's file properties.
set(IAT_AUTHOR "AuroralDiffraction (Tanatonium)")
string(TIMESTAMP IAT_YEAR "%Y")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(IAT_BUILD_TESTS "Build the unit tests and the UI smoke test" ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

if(MSVC)
    # Sources are UTF-8 (they contain characters such as the multiplication sign).
    add_compile_options(/utf-8 /W3 /permissive-)
endif()

# ---------------------------------------------------------------------------
# iacore: data model, processing and file I/O. No widgets (Qt Gui is used only
# to decode PNG and JPEG files), so the same code can later drive a
# command-line batch runner and be unit-tested.
# ---------------------------------------------------------------------------
add_library(iacore STATIC
    src/core/ElementType.h      src/core/ElementType.cpp
    src/core/Image2D.h          src/core/Image2D.cpp
    src/core/ImageStack.h       src/core/ImageStack.cpp
    src/core/Histogram.h        src/core/Histogram.cpp
    src/core/Processing.h       src/core/Processing.cpp
    src/io/MatFileReader.h      src/io/MatFileReader.cpp
    src/io/MatFileWriter.h      src/io/MatFileWriter.cpp
    src/io/TiffReader.h         src/io/TiffReader.cpp
    src/io/ImageFileReader.h    src/io/ImageFileReader.cpp
)
target_include_directories(iacore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated)
# Qt Gui only for QImage's PNG and JPEG decoders (no widgets; works headless).
target_link_libraries(iacore PUBLIC Qt6::Core Qt6::Gui)

# AppInfo.h: name, version, author and homepage from the values above.
configure_file(src/AppInfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/AppInfo.h @ONLY)

# ---------------------------------------------------------------------------
# iaui: Qt Widgets user interface.
# ---------------------------------------------------------------------------
add_library(iaui STATIC
    src/ui/MainWindow.h         src/ui/MainWindow.cpp
    src/ui/SessionTab.h         src/ui/SessionTab.cpp
    src/ui/ImagePanel.h         src/ui/ImagePanel.cpp
    src/ui/ImageView.h          src/ui/ImageView.cpp
    src/ui/HistogramWidget.h    src/ui/HistogramWidget.cpp
    src/ui/ContrastControl.h    src/ui/ContrastControl.cpp
    src/ui/AboutDialog.h        src/ui/AboutDialog.cpp
)
target_link_libraries(iaui PUBLIC iacore Qt6::Widgets Qt6::Concurrent)

# App icon as Qt resources (window title bar, taskbar, About box).
# The PNGs are generated from the SVGs by resources/icons/make_icons.py.
function(iat_add_icon_resources target)
    qt_add_resources(${target} "app_icons"
        PREFIX "/"
        BASE "${CMAKE_SOURCE_DIR}/resources"
        FILES
            "${CMAKE_SOURCE_DIR}/resources/icons/app_16.png"
            "${CMAKE_SOURCE_DIR}/resources/icons/app_24.png"
            "${CMAKE_SOURCE_DIR}/resources/icons/app_32.png"
            "${CMAKE_SOURCE_DIR}/resources/icons/app_48.png"
            "${CMAKE_SOURCE_DIR}/resources/icons/app_64.png"
            "${CMAKE_SOURCE_DIR}/resources/icons/app_128.png"
            "${CMAKE_SOURCE_DIR}/resources/icons/app_256.png")
endfunction()

# ---------------------------------------------------------------------------
# The application.
# ---------------------------------------------------------------------------
qt_add_executable(ImageAnalyzerTool WIN32 MACOSX_BUNDLE src/main.cpp)
target_link_libraries(ImageAnalyzerTool PRIVATE iaui)
iat_add_icon_resources(ImageAnalyzerTool)

# Windows: icon shown by Explorer / taskbar, and version info (author, URL)
# under the .exe's Properties > Details.
if(WIN32)
    enable_language(RC)
    set(IAT_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.ico")
    configure_file(src/app/ImageAnalyzerTool.rc.in ${CMAKE_CURRENT_BINARY_DIR}/generated/ImageAnalyzerTool.rc @ONLY)
    target_sources(ImageAnalyzerTool PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated/ImageAnalyzerTool.rc)
endif()

# ---------------------------------------------------------------------------
# "deploy" target (Windows): builds a folder you can give to anyone, plus a
# .zip of it, in <build folder>/dist:
#
#   dist/ImageAnalyzerTool-<version>/        ImageAnalyzerTool.exe + Qt DLLs,
#                                            plugins and compiler runtime
#   dist/ImageAnalyzerTool-<version>.zip
#
# The target PC needs nothing installed (no Qt, no MinGW, no MATLAB). Build the
# Release configuration first, so the package is small and fast.
# ---------------------------------------------------------------------------
set(IAT_QT_BIN_DIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
if(WIN32)
    find_program(IAT_WINDEPLOYQT windeployqt HINTS "${IAT_QT_BIN_DIR}")
    if(IAT_WINDEPLOYQT)
        set(IAT_DIST_NAME "ImageAnalyzerTool-${PROJECT_VERSION}")
        set(IAT_DIST_ROOT "${CMAKE_BINARY_DIR}/dist")
        set(IAT_DIST_DIR "${IAT_DIST_ROOT}/${IAT_DIST_NAME}")

        # The compiler's runtime DLLs go next to the .exe too.
        # MinGW: the GCC runtime, copied from the compiler's own folder, so
        # this works whatever is on PATH.
        # MSVC: the Visual C++ runtime (msvcp140.dll, vcruntime140.dll, ...).
        # Many PCs have it installed, but not all, and windeployqt only adds
        # it when run from a Visual Studio command prompt. CMake finds it in
        # the Visual Studio installation; Microsoft allows copying these DLLs
        # next to an application.
        set(_iat_runtime_commands)
        if(MINGW)
            get_filename_component(_iat_mingw_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
            foreach(_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
                if(EXISTS "${_iat_mingw_bin}/${_dll}")
                    list(APPEND _iat_runtime_commands
                         COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_iat_mingw_bin}/${_dll}" "${IAT_DIST_DIR}")
                endif()
            endforeach()
        elseif(MSVC)
            set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)  # only find them, no install() rules
            include(InstallRequiredSystemLibraries)
            foreach(_dll IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
                if(EXISTS "${_dll}")
                    list(APPEND _iat_runtime_commands
                         COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}" "${IAT_DIST_DIR}")
                endif()
            endforeach()
        endif()

        add_custom_target(deploy
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${IAT_DIST_DIR}" "${IAT_DIST_ROOT}/${IAT_DIST_NAME}.zip"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${IAT_DIST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ImageAnalyzerTool>" "${IAT_DIST_DIR}"
            # Qt DLLs and plugins. A Widgets app needs no software OpenGL or
            # Direct3D shader compiler, which keeps the package small.
            COMMAND "${IAT_WINDEPLOYQT}" --no-translations --no-opengl-sw --no-system-d3d-compiler
                    "${IAT_DIST_DIR}/$<TARGET_FILE_NAME:ImageAnalyzerTool>"
            ${_iat_runtime_commands}
            COMMAND ${CMAKE_COMMAND} -E chdir "${IAT_DIST_ROOT}"
                    ${CMAKE_COMMAND} -E tar cf "${IAT_DIST_NAME}.zip" --format=zip "${IAT_DIST_NAME}"
            COMMAND ${CMAKE_COMMAND} -E echo "Ready to share: ${IAT_DIST_ROOT}/${IAT_DIST_NAME}.zip"
            DEPENDS ImageAnalyzerTool
            COMMENT "Packaging ${IAT_DIST_NAME} (exe + Qt runtime) into ${IAT_DIST_ROOT}"
            VERBATIM)
    endif()
endif()

if(IAT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
