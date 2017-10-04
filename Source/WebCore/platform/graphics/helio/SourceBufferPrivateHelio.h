#ifndef SourceBufferPrivateHelio_h
#define SourceBufferPrivateHelio_h

#include "SourceBufferPrivate.h"
#include "MediaDescription.h"

#include "rcvmf_isobmff.h"

#include <wtf/WeakPtr.h>

//#include "helio.h"
//#include "demux/track.h"
//#include "demux/sample.h"

namespace WebCore {

class MediaSourcePrivateHelio;


class MediaDescriptionHelio final : public MediaDescription {
public:
    static RefPtr<MediaDescriptionHelio> create(char *codec) { return adoptRef(new MediaDescriptionHelio(codec)); }
    virtual ~MediaDescriptionHelio() { }

    AtomicString codec() const override { return m_codec; }
    bool isVideo() const override { return m_isVideo; }
    bool isAudio() const override { return m_isAudio; }
    bool isText() const override { return m_isText; }

protected:
    MediaDescriptionHelio(char *codec)
        : m_isVideo(strcmp(codec, "avc1") == 0)
        , m_isAudio(strcmp(codec, "mp4a") == 0)
        , m_isText(0)
        , m_codec(codec)
        { }

    AtomicString m_codec;
    bool m_isVideo;
    bool m_isAudio;
    bool m_isText;
};

class SourceBufferPrivateHelio final : public SourceBufferPrivate {
public:
    // TODO: Pass the codec here..
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
    //void trackInfoEventHandler(const SourceBufferPrivateClient::InitializationSegment& segment);

    //void mediaSampleEventHandler(helio_sample_t **samples);

    //void demuxCompleteEventHandler();
    //void setTrackDescription(PassRefPtr<MediaDescriptionHelio>);

private:

    explicit SourceBufferPrivateHelio(MediaSourcePrivateHelio*);

    void didDetectISOBMFFHeader(rcv_node_t *root);

    void didDetectISOBMFFSegment(rcv_node_t *root);

    WeakPtr<SourceBufferPrivateHelio> createWeakPtr() { return m_weakFactory.createWeakPtr(); }

    WeakPtrFactory<SourceBufferPrivateHelio> m_weakFactory;

    MediaPlayer::ReadyState m_readyState;

    SourceBufferPrivateClient* m_client;
    //helio_t *m_helio;
    //HashMap<AtomicString, bool> m_trackNotifyMap;
    rcv_parser_t *m_rcvmfParser;
    MediaSourcePrivateHelio *m_mediaSource;

    // RefPtr<MediaDescriptionHelio> m_trackDescription;
  
    // Captured from the MVHD box when the init segment is appended,
    // then provided to each MediaSample so that it can calculate it's
    // duration, cts & dts.
    uint32_t m_timescale;

};
}

#endif
