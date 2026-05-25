# install_ofx_bundle(TARGET <target> BUNDLE_NAME <name>)
# Creates the correct OFX bundle directory structure for macOS and Linux,
# and generates Info.plist on macOS.
function(install_ofx_bundle)
    cmake_parse_arguments(ARG "" "TARGET;BUNDLE_NAME" "" ${ARGN})

    if(APPLE)
        set(OFX_PLATFORM "MacOS")
    elseif(UNIX)
        set(OFX_PLATFORM "Linux-x86-64")
    elseif(WIN32)
        set(OFX_PLATFORM "Win64")
    endif()

    set(BUNDLE_CONTENT_DIR "${ARG_BUNDLE_NAME}.ofx.bundle/Contents")

    install(TARGETS ${ARG_TARGET}
        LIBRARY DESTINATION "${BUNDLE_CONTENT_DIR}/${OFX_PLATFORM}"
    )

    # Copy presets alongside the bundle
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/presets
        DESTINATION "${BUNDLE_CONTENT_DIR}/Resources"
    )

    if(APPLE)
        configure_file(
            ${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
            ${CMAKE_BINARY_DIR}/Info.plist
            @ONLY
        )
        install(FILES ${CMAKE_BINARY_DIR}/Info.plist
            DESTINATION "${BUNDLE_CONTENT_DIR}"
        )
    endif()
endfunction()
