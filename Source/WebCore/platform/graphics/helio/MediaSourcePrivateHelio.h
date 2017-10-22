#ifndef MediaSourceHelioPrivate_h
#define MediaSourceHelioPrivate_h

#include "MediaSourcePrivate.h"
#include "rcvmf_media_pipeline.h"

namespace WebCore {

class MediaPlayerPrivateHelio;
class SourceBufferPrivateHelio;

class MediaSourcePrivateHelio final : public MediaSourcePrivate {

public:
    //static RefPtr<MediaSourcePrivateAVFObjC> create(MediaPlayerPrivateMediaSourceAVFObjC*, MediaSourcePrivateClient*);
    static RefPtr<MediaSourcePrivateHelio> create(MediaPlayerPrivateHelio*, MediaSourcePrivateClient*);
    ~MediaSourcePrivateHelio();

    AddStatus addSourceBuffer(const ContentType&, RefPtr<SourceBufferPrivate>&) override;
    void durationChanged() override;

    void markEndOfStream(EndOfStreamStatus) override;
    void unmarkEndOfStream() override;

    /*
     W3C defines this as..
     https://www.w3.org/TR/media-source/#dom-readystate
     enum ReadyState {
     "closed",
     "open",
     "ended"
     };
     */
    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

    void waitForSeekCompleted() override;
    void seekCompleted() override;

    // When a SourceBuffer set's it's private impl (Helio) to active, it must report
    // back to the MediaPlayer though the MediaSource.
    void sourceBufferPrivateActiveStateChanged(SourceBufferPrivateHelio*,
                                               rcv_media_pipeline_t *);
    
    rcv_media_platform_t * mediaPlatform();

private:
    MediaSourcePrivateHelio(MediaPlayerPrivateHelio*, MediaSourcePrivateClient*);
    // TODO: Weird that this isn't defined on the
    // MediaSourcePrivate class as a pure virtual method
    //https://developer.mozilla.org/en-US/docs/Web/API/MediaSource/removeSourceBuffer
    void removeSourceBuffer(SourceBufferPrivate*);

    MediaPlayer::ReadyState m_readyState;

    HashMap<RefPtr<SourceBufferPrivateHelio>, rcv_media_pipeline_t *> m_sourceBuffers;

    MediaPlayerPrivateHelio *m_mediaPlayer;
    rcv_media_platform_t *m_mediaPlatform;
};

}

#endif
