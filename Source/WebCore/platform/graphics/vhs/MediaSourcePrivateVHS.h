#ifndef MediaSourcePrivateVHS_h
#define MediaSourcePrivateVHS_h

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include "MediaSourcePrivate.h"
#include "MediaClockReporter.h"

namespace WebCore {

// class MediaPlayerPrivateVHS;
class SourceBufferPrivateVHS;
class DataLoaderProxy;
class ReadyStateResponder;
class VHSMediaPipeline;
class MediaPlatformPrivate;

/**
 * NOTE(estobbart): The VHS platofrm impl currently only supports the source
 * buffer configuration where two source buffers exist, each handling a single
 * audio track & video track.
 *
 * https://w3c.github.io/media-source/#sourcebuffer-configuration
 *
 * When SourceBuffers first become active, they connect their vhs_media_pipeline
 * to the vhs_media_platform via the VHSMediaPlatform instance.
 */
class MediaSourcePrivateVHS final : public MediaSourcePrivate, public MediaClockDataSource {

public:
    static Ref<MediaSourcePrivateVHS> create(MediaClockReporter*, ReadyStateResponder*);
    virtual ~MediaSourcePrivateVHS();

    // MediaSourcePrivate overrides start --
    AddStatus addSourceBuffer(const ContentType&, RefPtr<SourceBufferPrivate>&) override;
    void durationChanged() override;

    void markEndOfStream(EndOfStreamStatus) override;
    void unmarkEndOfStream() override;


    // NOTE(estobbart): https://www.w3.org/TR/media-source/#dom-readystate
    // MediaSource readyState { 'closed', 'open', 'ended' } is different from
    // MediaSourcePrivate readyState, which is the source of truth for the
    // HTMLMediaElement.readyState in the MSE scenario.
    // State changes can come from both the MediaSource & the DataLoaderProxy
    // Any change must be reported back to the MediaPlayer.
    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

    void waitForSeekCompleted() override;
    void seekCompleted() override;
    // MediaSourcePrivate overrides end --

    // The MediaPlayer needs this ??
    // rcv_media_platform_t * mediaPlatform();

    // MediaClockDataSource overrides
    float lastPresentedTimestamp() override;

    // TODO: clean up all these interfaces and objects..
    DataLoaderProxy* dataLoaderProxy() const { return m_dataLoaderProxy.get(); }

    MediaPlatformPrivate* mediaPlatform() const { return m_vhsMediaPlatform.get(); }

private:
    MediaSourcePrivateVHS(MediaClockReporter*, ReadyStateResponder*);

    // TODO: Weird that this isn't defined on the
    // MediaSourcePrivate class as a pure virtual method
    //https://developer.mozilla.org/en-US/docs/Web/API/MediaSource/removeSourceBuffer
    // This looks like something gstreamer did, this doesn't actually get called.
    // The MediaSource removes it from the m_sourceBuffers & m_activeSourceBuffers
    // lists, and then calls SourceBufferPrivate::removedFromMediaSource().
    void removeSourceBuffer(SourceBufferPrivate*);

    RefPtr<MediaPlatformPrivate> m_vhsMediaPlatform;

    MediaPlayer::ReadyState m_readyState;

    MediaClockReporter *m_clockReporter;
    ReadyStateResponder *m_readyStateResponder;

    // CAuses a memory leak..
    // HashMap<RefPtr<SourceBufferPrivateVHS>, rcv_media_pipeline_t *> m_sourceBuffers;

    // MediaPlayerPrivateVHS *m_mediaPlayer;
    // rcv_media_platform_t *m_mediaPlatform;

    // uint8_t m_pendingBufferActivation;
    RefPtr<DataLoaderProxy> m_dataLoaderProxy;
};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)

#endif
