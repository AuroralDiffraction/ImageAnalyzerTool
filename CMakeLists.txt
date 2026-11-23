cmake_minimum_required(VERSION 3.19)

project(ImageAnalyzerTool
    VERSION 0.5.0
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

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Concurrent LinguistTools)
find_package(Threads REQUIRED)  # the image processing stages use several threads

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
    src/core/Isp.h              src/core/Isp.cpp
    src/core/Hsi.h              src/core/Hsi.cpp
    src/core/HsiLinalg.h        src/core/HsiLinalg.cpp
    src/core/HsiColour.h        src/core/HsiColour.cpp
    src/core/HsiAnalysis.h      src/core/HsiAnalysis.cpp
    src/core/HsiCorrection.h    src/core/HsiCorrection.cpp
    src/io/MatFileReader.h      src/io/MatFileReader.cpp
    src/io/MatFileWriter.h      src/io/MatFileWriter.cpp
    src/io/TiffReader.h         src/io/TiffReader.cpp
    src/io/ImageFileReader.h    src/io/ImageFileReader.cpp
    src/io/HsiCubeIo.h          src/io/HsiCubeIo.cpp
)
target_include_directories(iacore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated)
# Qt Gui only for QImage's PNG and JPEG decoders (no widgets; works headless).
target_link_libraries(iacore PUBLIC Qt6::Core Qt6::Gui Threads::Threads)

# AppInfo.h: name, version, author and homepage from the values above.
configure_file(src/AppInfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/AppInfo.h @ONLY)

# ---------------------------------------------------------------------------
# iaui: Qt Widgets user interface.
# ---------------------------------------------------------------------------
add_library(iaui STATIC
    src/ui/MainWindow.h         src/ui/MainWindow.cpp
    src/ui/SessionTab.h         src/ui/SessionTab.cpp
    src/ui/LayoutMode.h
    src/ui/ModeSwitch.h         src/ui/ModeSwitch.cpp
    src/ui/Language.h           src/ui/Language.cpp
    src/ui/Theme.h              src/ui/Theme.cpp
    src/ui/Icons.h              src/ui/Icons.cpp
    src/ui/SettingsDialog.h     src/ui/SettingsDialog.cpp
    src/ui/ImagePanel.h         src/ui/ImagePanel.cpp
    src/ui/PipelineDialog.h     src/ui/PipelineDialog.cpp
    src/ui/ImageView.h          src/ui/ImageView.cpp
    src/ui/HistogramWidget.h    src/ui/HistogramWidget.cpp
    src/ui/ContrastControl.h    src/ui/ContrastControl.cpp
    src/ui/AboutDialog.h        src/ui/AboutDialog.cpp
    src/ui/HsiWorkspace.h       src/ui/HsiWorkspace.cpp
    src/ui/SpectrumPlot.h       src/ui/SpectrumPlot.cpp
    src/ui/ScorePlot.h          src/ui/ScorePlot.cpp
    src/ui/ImportImageDialog.h  src/ui/ImportImageDialog.cpp
    src/ui/MosaicLayoutDialog.h src/ui/MosaicLayoutDialog.cpp
    src/ui/CorrectionDialog.h   src/ui/CorrectionDialog.cpp
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
# Translations. The texts in the code are English; every .ts file listed here
# is one more interface language (Chinese is the default, see
# src/ui/Language.cpp). lrelease compiles each .ts into a .qm file, which is
# embedded in the executables as :/i18n/ImageAnalyzerTool_<code>.qm.
#
# After changing texts in the code, bring the .ts files up to date with
#   cmake --build <build folder> --target update_translations
# and translate the new entries (Qt Linguist, or edit the .ts file).
# ---------------------------------------------------------------------------
set(IAT_TS_FILES
    "${CMAKE_SOURCE_DIR}/translations/ImageAnalyzerTool_zh_CN.ts")

# The target that compiles the .ts files is IAT_LRELEASE_TARGET. Qt 6.7
# changed how qt_add_lrelease names it: up to Qt 6.6 the form used here,
# qt_add_lrelease(<target> ...), makes <target>_lrelease; from Qt 6.7 that
# form is deprecated and names it <project>_lrelease, and the name is set
# with LRELEASE_TARGET instead.
if(Qt6LinguistTools_VERSION VERSION_GREATER_EQUAL 6.7)
    set(IAT_LRELEASE_TARGET iat_lrelease)
    qt_add_lrelease(TS_FILES ${IAT_TS_FILES}
        LRELEASE_TARGET ${IAT_LRELEASE_TARGET}
        QM_FILES_OUTPUT_VARIABLE IAT_QM_FILES)
else()
    qt_add_lrelease(iaui TS_FILES ${IAT_TS_FILES} QM_FILES_OUTPUT_VARIABLE IAT_QM_FILES)
    set(IAT_LRELEASE_TARGET iaui_lrelease)
endif()
if(NOT TARGET ${IAT_LRELEASE_TARGET})
    message(FATAL_ERROR "qt_add_lrelease did not create the target ${IAT_LRELEASE_TARGET} "
                        "(Qt LinguistTools ${Qt6LinguistTools_VERSION}).")
endif()

# Qt's own texts (standard buttons, About Qt, ...) in the same languages, when
# this Qt installation has them.
set(IAT_QT_QM_FILES)
foreach(_ts IN LISTS IAT_TS_FILES)
    get_filename_component(_name "${_ts}" NAME_WE)                # ImageAnalyzerTool_zh_CN
    string(REPLACE "ImageAnalyzerTool_" "" _lang "${_name}")      # zh_CN
    set(_qt_qm "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}/qtbase_${_lang}.qm")
    if(EXISTS "${_qt_qm}")
        list(APPEND IAT_QT_QM_FILES "${_qt_qm}")
    endif()
endforeach()

function(iat_add_translation_resources target)
    foreach(_qm IN LISTS IAT_QM_FILES IAT_QT_QM_FILES)
        get_filename_component(_alias "${_qm}" NAME)
        set_source_files_properties("${_qm}" PROPERTIES QT_RESOURCE_ALIAS "${_alias}")
    endforeach()
    # The .qm files are made by the lrelease target (in the top-level folder).
    set_source_files_properties(${IAT_QM_FILES} PROPERTIES GENERATED TRUE)
    qt_add_resources(${target} "translations"
        PREFIX "/i18n"
        FILES ${IAT_QM_FILES} ${IAT_QT_QM_FILES})
    add_dependencies(${target} ${IAT_LRELEASE_TARGET})
endfunction()

# The update_translations target: rewrites the .ts files from the texts in the
# code (iaui, iacore and main.cpp), keeping the existing translations.
# Texts that are no longer in the code stay in the .ts file, marked as
# vanished, until they are removed in Qt Linguist.
#
# lupdate must read the sources with the targets' include paths, or it does
# not see that the classes are in namespace iat and files the texts under
# the wrong names. Up to Qt 6.6, qt_add_lupdate(<target> SOURCES ...) took the
# include paths from <target>; from Qt 6.7 sources listed under SOURCES get
# none, so the targets are listed under SOURCE_TARGETS instead (main.cpp
# names its context, "main", itself).
if(Qt6LinguistTools_VERSION VERSION_GREATER_EQUAL 6.7)
    qt_add_lupdate(TS_FILES ${IAT_TS_FILES}
        SOURCE_TARGETS iaui iacore
        SOURCES src/main.cpp
        INCLUDE_DIRECTORIES src ${CMAKE_CURRENT_BINARY_DIR}/generated
        OPTIONS -locations relative)
else()
    get_target_property(_iat_ui_sources iaui SOURCES)
    get_target_property(_iat_core_sources iacore SOURCES)
    qt_add_lupdate(iaui TS_FILES ${IAT_TS_FILES}
        SOURCES ${_iat_ui_sources} ${_iat_core_sources} src/main.cpp
        OPTIONS -locations relative)
endif()

# ---------------------------------------------------------------------------
# The application.
# ---------------------------------------------------------------------------
qt_add_executable(ImageAnalyzerTool WIN32 MACOSX_BUNDLE src/main.cpp)
target_link_libraries(ImageAnalyzerTool PRIVATE iaui)
iat_add_icon_resources(ImageAnalyzerTool)
iat_add_translation_resources(ImageAnalyzerTool)

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
