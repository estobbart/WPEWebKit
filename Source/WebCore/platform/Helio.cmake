if (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/helio"
    )

    list(APPEND WebCore_SOURCES
        platform/graphics/helio/MediaPlayerPrivateHelio.cpp
        platform/graphics/helio/MediaSourcePrivateHelio.cpp
        platform/graphics/helio/SourceBufferPrivateHelio.cpp
        platform/graphics/helio/MediaSampleHelio.cpp
    )

    list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
        /home/estobb200/Development/RDK/rpi-yocto/libhelio/include
    )

#    link_directories("/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib" "${WEBKIT_LIBRARIES_LINK_DIR}")

    list(APPEND WebCore_LIBRARIES
        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
    )

#set(WebKit2_OUTPUT_NAME WPEWebKit)
#set(WebKit2_WebProcess_OUTPUT_NAME WPEWebProcess)
#set(WebKit2_NetworkProcess_OUTPUT_NAME WPENetworkProcess)
#set(WebKit2_DatabaseProcess_OUTPUT_NAME WPEDatabaseProcess)

    list(APPEND WebKit2_LIBRARIES
        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
    )

    list(APPEND WebProcess_LIBRARIES
        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
    )

    list(APPEND WebKit2_WebProcess_LIBRARIES
        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
    )

    list(APPEND WebKit2_NetworkProcess_LIBRARIES
        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
    )

    list(APPEND WebKit2_DatabaseProcess_LIBRARIES
        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
    )

endif ()
