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
endif ()
