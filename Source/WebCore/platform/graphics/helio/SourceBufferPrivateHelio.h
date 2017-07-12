#ifndef SourceBufferPrivateHelio_h
#define SourceBufferPrivateHelio_h

#include "SourceBufferPrivate.h"

#include "helio.h"
#include "demux/track.h"
#include "demux/sample.h"

namespace WebCore {

class MediaSourcePrivateHelio;

class SourceBufferPrivateHelio final : public SourceBufferPrivate {
public:
    static RefPtr<SourceBufferPrivateHelio> create(MediaSourcePrivateHelio*);
    ~SourceBufferPrivateHelio();

    void setClient(SourceBufferPrivateClient*) override;

    void append(const unsigned char*, unsigned) override;
    void abort() override;
    void resetParserState() override;
    void removedFromMediaSource() override;

    //     enum ReadyState  { HaveNothing, HaveMetadata, HaveCurrentData, HaveFutureData, HaveEnoughData };
    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

    // Called by the SourceBuffer during reenqueueMediaForTime
    // TODO: (estobb200): Need to undersatnd the usage of this function better
    void flush(AtomicString) override;

    // Called by the SourceBuffer during provideMediaData after
    // it checks if we're ready for more samples, and we return true.
    // As soon as a sample is enqueued, it is erased from the decodeQueue
    // of the trackBuffer. NOTE(estobb200): At this point the memory
    // pointed to by the MediaSample shold be considered owned by libhelio
    // again.
    void enqueueSample(PassRefPtr<MediaSample>, AtomicString) override;

    // Called by the SourceBuffer during provideMediaData before calling
    // enqueueSample. provideMediaData generally occurs as soon as the
    // SoureBufferPrivate notifies it's client that it is done via the
    // sourceBufferPrivateAppendComplete method of the client.
    bool isReadyForMoreSamples(AtomicString) override;

    // Called by the SourceBuffer when it's own active flag gets set,
    // which seems to be tied to track changes, or after the initialization
    // segment is returned to the client.
    void setActive(bool) override;

    // Called by the SourceBuffer during provideMediaData after
    // it checks if we're ready for more samples, and we return false.
    void notifyClientWhenReadyForMoreSamples(AtomicString) override;

    // TODO: Is there a better way to hide these?
    void trackInfoEventHandler(helio_track_t * tracks, uint8_t track_count);

    void mediaSampleEventHandler(helio_sample_t *sample);

    void demuxCompleteEventHandler();

private:

    explicit SourceBufferPrivateHelio(MediaSourcePrivateHelio*);
    MediaPlayer::ReadyState m_readyState;

    SourceBufferPrivateClient* m_client;
    helio_t *m_helio;

};
}

#endif
