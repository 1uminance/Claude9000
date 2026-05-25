cmake_minimum_required(VERSION 3.24)
project(FilmTools VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── OFX SDK (vendored) ─────────────────────────────────────────────────────────
# Cloned from https://github.com/ofxa/openfx into vendor/openfx
# We only use the headers and support library sources — the OFX SDK's own
# CMakeLists.txt is intentionally not processed.
set(openfx_SOURCE_DIR "${CMAKE_SOURCE_DIR}/vendor/openfx")

# ── OFX support library (compiled into the plugin, not a separate shared lib) ──
set(OFX_SUPPORT_SOURCES
    ${openfx_SOURCE_DIR}/Support/Library/ofxsCore.cpp
    ${openfx_SOURCE_DIR}/Support/Library/ofxsImageEffect.cpp
    ${openfx_SOURCE_DIR}/Support/Library/ofxsInteract.cpp
    ${openfx_SOURCE_DIR}/Support/Library/ofxsLog.cpp
    ${openfx_SOURCE_DIR}/Support/Library/ofxsMultiThread.cpp
    ${openfx_SOURCE_DIR}/Support/Library/ofxsParams.cpp
    ${openfx_SOURCE_DIR}/Support/Library/ofxsProperty.cpp
    ${openfx_SOURCE_DIR}/Support/Library/ofxsPropertyValidation.cpp
)

set(OFX_INCLUDE_DIRS
    ${openfx_SOURCE_DIR}/include
    ${openfx_SOURCE_DIR}/Support/include
)

# ── Plugin sources ─────────────────────────────────────────────────────────────
set(FILMTOOLS_SOURCES
    src/PluginMain.cpp
    src/common/ImageProcessor.cpp
    src/FilmSplit/FilmSplitPlugin.cpp
    src/FilmCombine/FilmCombinePlugin.cpp
    ${OFX_SUPPORT_SOURCES}
)

# ── Shared library target ──────────────────────────────────────────────────────
add_library(FilmTools MODULE ${FILMTOOLS_SOURCES})

target_include_directories(FilmTools PRIVATE
    ${OFX_INCLUDE_DIRS}
    src/common
    src/FilmSplit
    src/FilmCombine
)

set_target_properties(FilmTools PROPERTIES
    PREFIX      ""
    OUTPUT_NAME "FilmTools"
    SUFFIX      ".ofx"
)

# ── Platform-specific settings ─────────────────────────────────────────────────
if(APPLE)
    set_target_properties(FilmTools PROPERTIES
        OSX_ARCHITECTURES "arm64;x86_64"
        LIBRARY_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/FilmTools.ofx.bundle/Contents/MacOS"
    )
    target_link_options(FilmTools PRIVATE
        -exported_symbols_list ${CMAKE_SOURCE_DIR}/cmake/ofx_exports.sym
    )
elseif(UNIX)
    set_target_properties(FilmTools PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/FilmTools.ofx.bundle/Contents/Linux-x86-64"
    )
    target_link_options(FilmTools PRIVATE
        -Wl,--version-script=${CMAKE_SOURCE_DIR}/cmake/ofx_exports.lds
        -Wl,-z,defs
    )
    target_link_libraries(FilmTools PRIVATE pthread)
endif()

# ── Bundle install layout ──────────────────────────────────────────────────────
include(cmake/OFXBundle.cmake)
install_ofx_bundle(TARGET FilmTools BUNDLE_NAME FilmTools)
