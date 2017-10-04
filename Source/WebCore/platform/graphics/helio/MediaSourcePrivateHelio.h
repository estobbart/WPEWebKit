#ifndef MediaSourceHelioPrivate_h
#define MediaSourceHelioPrivate_h

#include "MediaSourcePrivate.h"

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

    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

    void waitForSeekCompleted() override;
    void seekCompleted() override;

    // When a SourceBuffer set's it's private impl (Helio) to active, it must report
    // back to the MediaPlayer though the MediaSource.
    void sourceBufferPrivateActiveStateChanged(bool isActive);

private:
    MediaSourcePrivateHelio(MediaPlayerPrivateHelio*, MediaSourcePrivateClient*);
    // TODO: Weird that this isn't defined on the
    // MediaSourcePrivate class as a pure virtual method
    //https://developer.mozilla.org/en-US/docs/Web/API/MediaSource/removeSourceBuffer
    void removeSourceBuffer(SourceBufferPrivate*);

    MediaPlayer::ReadyState m_readyState;

    Vector<RefPtr<SourceBufferPrivateHelio>> m_sourceBuffers;
    MediaPlayerPrivateHelio *m_mediaPlayer;
};

}

#endif
