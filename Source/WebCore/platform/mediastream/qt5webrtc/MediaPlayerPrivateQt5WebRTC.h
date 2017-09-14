/*
* Copyright (c) 2016, Comcast
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR OR; PROFITS BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY OF THEORY LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MediaPlayerPrivateQt5WebRTC_h
#define MediaPlayerPrivateQt5WebRTC_h

#include "MediaPlayerPrivate.h"
#include "IntRect.h"
#include "FloatRect.h"

#include "RealtimeMediaSourceCenterQt5WebRTC.h"

#if USE(COORDINATED_GRAPHICS_THREADED)
#include "TextureMapperPlatformLayerProxy.h"
#endif

namespace WebCore {

class MediaPlayerPrivateQt5WebRTC : public MediaPlayerPrivateInterface
    , public WRTCInt::RTCVideoRendererClient
    , public RealtimeMediaSource::Observer
#if USE(COORDINATED_GRAPHICS_THREADED)
    , public TextureMapperPlatformLayerProxyProvider
#endif
{
public:
    explicit MediaPlayerPrivateQt5WebRTC(MediaPlayer*);
    ~MediaPlayerPrivateQt5WebRTC();

    static void registerMediaEngine(MediaEngineRegistrar);

    bool hasVideo() const override { return true; }
    bool hasAudio() const override { return true; }

    void load(const String&) override { notifyLoadFailed(); }
#if ENABLE(MEDIA_SOURCE)
    void load(const String&, MediaSourcePrivateClient*) override { notifyLoadFailed(); }
#endif
#if ENABLE(MEDIA_STREAM)
    void load(MediaStreamPrivate&) override;
#endif
    void commitLoad() { }
    void cancelLoad() override { }

    void play() override;
    void pause() override;
    bool paused() const override { return m_paused; }
    bool seeking() const override { return false; }
    std::unique_ptr<PlatformTimeRanges> buffered() const override { return std::make_unique<PlatformTimeRanges>(); }
    bool didLoadingProgress() const override { return false; }
    void setVolume(float) override { }
    float volume() const override { return 0; }
    bool supportsMuting() const override { return true; }
    void setMuted(bool) override { }
    void setVisible(bool) override;
    void setSize(const IntSize&) override;
    void setPosition(const IntPoint&) override;
    void paint(GraphicsContext&, const FloatRect&) override { }

    MediaPlayer::NetworkState networkState() const override {return m_networkState; }
    MediaPlayer::ReadyState readyState() const { return m_readyState; }

    FloatSize naturalSize() const override;

    bool supportsAcceleratedRendering() const override { return true; }

#if USE(COORDINATED_GRAPHICS_THREADED)
    PlatformLayer* platformLayer() const override { return const_cast<MediaPlayerPrivateQt5WebRTC*>(this); }
    RefPtr<TextureMapperPlatformLayerProxy> proxy() const override { return m_platformLayerProxy.copyRef(); }
    void swapBuffersIfNeeded() override { }
#else
    PlatformLayer* platformLayer() const override { return nullptr; }
#endif

    // RealtimeMediaSource::Observer
    void sourceStopped() override;
    void sourceMutedChanged() override;
    void sourceSettingsChanged() override;
    bool preventSourceFromStopping() override { return false; }
    // Media data changes.
    virtual void sourceHasMoreMediaData(MediaSample&) override { };

    // WRTCInt::VideoPlayerClient
    void renderFrame(const unsigned char *data, int byteCount, int width, int height) override;
    void punchHole(int width, int height) override;

  private:
    void notifyLoadFailed();
    static void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>&);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);
    void updateVideoRectangle();
    void tryAttachRenderer();
    void removeRenderer();

#if USE(COORDINATED_GRAPHICS_THREADED)
    void pushTextureToCompositor(RefPtr<Image> frame);
    RefPtr<TextureMapperPlatformLayerProxy> m_platformLayerProxy;
    RefPtr<GraphicsContext3D> m_context3D;
    Condition m_drawCondition;
    Lock m_drawMutex;
#endif
    std::unique_ptr<WRTCInt::RTCVideoRenderer> m_rtcRenderer;

    MediaPlayer* m_player;
    MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;
    IntSize m_size;
    IntPoint m_position;
    MediaStreamPrivate* m_stream {nullptr};
    bool m_paused {true};
};

}
#endif
