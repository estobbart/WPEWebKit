#ifndef SourceBufferPrivateHelio_h
#define SourceBufferPrivateHelio_h

#include "SourceBufferPrivate.h"

namespace WebCore {

class MediaSourcePrivateHelio;

class SourceBufferPrivateHelio final : public SourceBufferPrivate {
public:
    // SourceBufferPrivateHelio();
    //static RefPtr<MockSourceBufferPrivate> create(MockMediaSourcePrivate*);
    //static Ref<SourceBufferPrivateGStreamer> create(MediaSourceGStreamer*, Ref<MediaSourceClientGStreamerMSE>, const ContentType&);
    //static RefPtr<SourceBufferPrivateAVFObjC> create(MediaSourcePrivateAVFObjC*);
    static RefPtr<SourceBufferPrivateHelio> create(MediaSourcePrivateHelio*);
    ~SourceBufferPrivateHelio();

    void setClient(SourceBufferPrivateClient*) override;

    void append(const unsigned char*, unsigned) override;
    void abort() override;
    void resetParserState() override;
    void removedFromMediaSource() override;

    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;
private:
    explicit SourceBufferPrivateHelio(MediaSourcePrivateHelio*);
    MediaPlayer::ReadyState m_readyState;

    SourceBufferPrivateClient* m_client;
};
}

#endif
