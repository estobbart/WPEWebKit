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
        /home/estobb200/Development/RDK/librcvmf/utils/
        /home/estobb200/Development/RDK/librcvmf/task/include/
    )

#    link_directories("/home/estobb200/Development/RDK/rpi-yocto/libhelio/build/lib" "${WEBKIT_LIBRARIES_LINK_DIR}") // doesn't work

    list(APPEND WebCore_LIBRARIES
        -lnexus_static
        -lnxclient
        -L/home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/work/cortexa15t2hf-vfp-neon-rdk-linux-gnueabi/rcvmf/1.0-r0/rcvmf-1.0/isobmff/ -lisobmff
        -L/home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/work/cortexa15t2hf-vfp-neon-rdk-linux-gnueabi/rcvmf/1.0-r0/rcvmf-1.0/engine/ -lrcvmf
        -L/home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/work/cortexa15t2hf-vfp-neon-rdk-linux-gnueabi/rcvmf/1.0-r0/rcvmf-1.0/utils/ -lrcvmfutils
        -L/home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/work/cortexa15t2hf-vfp-neon-rdk-linux-gnueabi/rcvmf/1.0-r0/rcvmf-1.0/task/ -lrcvmftask
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

if (ENABLE_LEGACY_ENCRYPTED_MEDIA_V1 OR ENABLE_LEGACY_ENCRYPTED_MEDIA OR ENABLE_ENCRYPTED_MEDIA)
#    list(APPEND WebCore_INCLUDE_DIRECTORIES
#        ${LIBGCRYPT_INCLUDE_DIRS}
#    )
#    list(APPEND WebCore_LIBRARIES
#        ${LIBGCRYPT_LIBRARIES} -lgpg-error
#    )

#    if (ENABLE_PLAYREADY)

#        add_definitions(-DDRM_BUILD_PROFILE=DRM_BUILD_PROFILE_OEM)
#        add_definitions(-DTARGET_LITTLE_ENDIAN=1)
#        add_definitions(-DTARGET_SUPPORTS_UNALIGNED_DWORD_POINTERS=0)


#add_definitions(
#        -DDRM_SUPPORT_SECUREOEMHAL=1
#        -DDRM_SUPPORT_ECCPROFILING=0
#        -DDRM_SUPPORT_INLINEDWORDCPY=0
#        -DDRM_SUPPORT_DATASTORE_PREALLOC=1
#        -DDRM_SUPPORT_NATIVE_64BIT_TYPES=1
#        -DDRM_SUPPORT_TRACING=0
#        -D_DATASTORE_WRITE_THRU=1
#        -DDRM_HDS_COPY_BUFFER_SIZE=32768
#        -DDRM_TEST_SUPPORT_ACTIVATION=0
#        -DDRM_SUPPORT_REACTIVATION=0
#        -DDRM_SUPPORT_ASSEMBLY=0
#        -DDRM_SUPPORT_CRT=1
#        -DDRM_SUPPORT_DEVICE_SIGNING_KEY=0
#        -DDRM_SUPPORT_EMBEDDED_CERTS=0
#        -DDRM_SUPPORT_FORCE_ALIGN=0
#        -DDRM_SUPPORT_LOCKING=0
#        -DDRM_SUPPORT_MULTI_THREADING=0
#        -DDRM_SUPPORT_NONVAULTSIGNING=1
#        -DDRM_SUPPORT_PRECOMPUTE_GTABLE=0
#        -DDRM_SUPPORT_PRIVATE_KEY_CACHE=0
#        -DDRM_SUPPORT_SYMOPT=1
#        -DDRM_SUPPORT_XMLHASH=1
#        -DDRM_USE_IOCTL_HAL_GET_DEVICE_INFO=0
#        -DDRM_TEST_SUPPORT_NET_IO=0
#        -DUSE_PK_NAMESPACES=0
#        -DDRM_INCLUDE_PK_NAMESPACE_USING_STATEMENT=0
#        -D_ADDLICENSE_WRITE_THRU=0
#        -DDRM_KEYFILE_VERSION=3
#        -DDRM_SUPPORT_SECUREOEMHAL_PLAY_ONLY=1
#        -DARCLOCK=1
#)

#        list(APPEND WebCore_LIBRARIES
#             -lIARMBus
#            -lplayready
#            -lsystemd
#            -liarmmgrs
#            ${PLAYREADY_LIBRARIES}
#        )

#        list(APPEND WebCore_INCLUDE_DIRECTORIES
#            /home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/sysroots/pacexi5/usr/include/playready/
#            /home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/sysroots/pacexi5/usr/include/playready/oem/common/inc/
#            /home/estobb200/Development/RDK/Xi5/build-pacexi5/tmp/sysroots/pacexi5/usr/include/playready/oem/ansi/inc/
#            ${PLAYREADY_INCLUDE_DIRS}
#        )
        add_definitions(${PLAYREADY_CFLAGS_OTHER})
        foreach(p ${PLAYREADY_INCLUDE_DIRS})
            if (EXISTS "${p}/playready.cmake")
                include("${p}/playready.cmake")
            endif()
        endforeach()
        list(APPEND WebCore_INCLUDE_DIRECTORIES
            "${WEBCORE_DIR}/platform/graphics/helio/playready"
        )
        list(APPEND WebCore_SOURCES
            platform/graphics/helio/playready/PlayreadySession.cpp
        )
#    endif ()

endif ()
