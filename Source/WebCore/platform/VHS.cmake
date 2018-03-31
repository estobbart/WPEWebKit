if (ENABLE_VIDEO)

# TODO: Move this into a FindVHS.cmake file..
    find_package(PkgConfig)
    pkg_check_modules(VHS REQUIRED vhs)

    list(APPEND WebCore_LIBRARIES
        ${VHS_LIBRARIES}
    )

    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${VHS_INCLUDE_DIRS}
    )

    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/vhs"
    )

    list(APPEND WebCore_SOURCES
        platform/graphics/vhs/MediaPlayerPrivateVHS.cpp
        platform/graphics/vhs/MediaClockReporter.cpp
        platform/graphics/vhs/MediaSourcePrivateVHS.cpp
        platform/graphics/vhs/SourceBufferPrivateVHS.cpp
        platform/graphics/vhs/AudioTrackPrivateVHS.cpp
        platform/graphics/vhs/VideoTrackPrivateVHS.cpp
        platform/graphics/vhs/DataLoaderProxy.cpp
        platform/graphics/vhs/ReadyStateResponder.cpp
        platform/graphics/vhs/MediaSampleVHS.cpp
        platform/graphics/vhs/InitializationSegmentVHS.cpp
    )

endif ()
