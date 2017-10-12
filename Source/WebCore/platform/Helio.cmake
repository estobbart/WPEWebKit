if (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/helio"
    )

    list(APPEND WebCore_SOURCES
        platform/graphics/helio/MediaPlayerPrivateHelio.cpp
        platform/graphics/helio/MediaSourcePrivateHelio.cpp
        platform/graphics/helio/SourceBufferPrivateHelio.cpp
        platform/graphics/helio/MediaSampleHelio.cpp
        platform/graphics/helio/AudioTrackPrivateHelio.cpp
        platform/graphics/helio/VideoTrackPrivateHelio.cpp
    )

    list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
        /home/estobb200/Development/RDK/librcvmf/isobmff/include/
        /home/estobb200/Development/RDK/librcvmf/engine/include/
    )

#    link_directories("/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib" "${WEBKIT_LIBRARIES_LINK_DIR}") // doesn't work

    list(APPEND WebCore_LIBRARIES
        -lnexus_static
        -lnxclient
        -L/home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/work/cortexa15t2hf-vfp-neon-rdk-linux-gnueabi/rcvmf/1.0-r0/rcvmf-1.0/isobmff/ -lisobmff
        -L/home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/work/cortexa15t2hf-vfp-neon-rdk-linux-gnueabi/rcvmf/1.0-r0/rcvmf-1.0/engine/ -lrcvmf
    )

#set(WebKit2_OUTPUT_NAME WPEWebKit)
#set(WebKit2_WebProcess_OUTPUT_NAME WPEWebProcess)
#set(WebKit2_NetworkProcess_OUTPUT_NAME WPENetworkProcess)
#set(WebKit2_DatabaseProcess_OUTPUT_NAME WPEDatabaseProcess)

#    list(APPEND WebKit2_LIBRARIES
#        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
#    )

#    list(APPEND WebProcess_LIBRARIES
#        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
#    )

#    list(APPEND WebKit2_WebProcess_LIBRARIES
#        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
#    )

#    list(APPEND WebKit2_NetworkProcess_LIBRARIES
#        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
#    )

#    list(APPEND WebKit2_DatabaseProcess_LIBRARIES
#        -L/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib -lhelio
#    )

endif ()

if (USE_HOLE_PUNCH_EXTERNAL)
    list(APPEND WebCore_SOURCES
        platform/graphics/holepunch/MediaPlayerPrivateHolePunchBase.cpp
        platform/graphics/holepunch/MediaPlayerPrivateHolePunchDummy.cpp
    )

    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/holepunch"
    )
endif ()
