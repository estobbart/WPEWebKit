#ifndef SourceBufferPrivateHelio_h
#define SourceBufferPrivateHelio_h

#include "SourceBufferPrivate.h"
#include "MediaDescription.h"

#include "rcvmf_isobmff.h"
#include "rcvmf_media_pipeline.h"

#include "GenericTaskQueue.h"

#include <wtf/WeakPtr.h>


//#include "helio.h"
//#include "demux/track.h"
//#include "demux/sample.h"

namespace WebCore {

class MediaPlayerPrivateHelio;
class MediaSourcePrivateHelio;
class MediaSampleHelio;
class HelioCodecConfiguration;

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
        {
            if (!m_isAudio && !m_isVideo) {
                printf("ERROR creating MediaDescriptionHelio from %s \n", codec);
            }
        }

    AtomicString m_codec;
    bool m_isVideo;
    bool m_isAudio;
    bool m_isText;
};

class SourceBufferPrivateHelio final : public SourceBufferPrivate {
public:
    // TODO: Pass the expected mimetype/codec here..
    // currently we just write codec.
    static RefPtr<SourceBufferPrivateHelio> create(MediaPlayerPrivateHelio *,
                                                   MediaSourcePrivateHelio *,
                                                   String);
    ~SourceBufferPrivateHelio();

    void setClient(SourceBufferPrivateClient*) override;

    void append(const unsigned char*, unsigned) override;
    void abort() override;
    void resetParserState() override;
    void removedFromMediaSource() override;

    // enum ReadyState  { HaveNothing, HaveMetadata, HaveCurrentData, HaveFutureData, HaveEnoughData };
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
    // again. <- this isn't 100%, it's possibly that it's still in a buffer
    // until remove is called.
    void enqueueSample(PassRefPtr<MediaSample>, AtomicString) override;
    // void enqueueSample(Ref<MediaSample>&&, AtomicString) override;

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

    // Called by the MediaSource once all the created buffer's become active.
    void startStream();

    // callback from rcvmf
    void platformBufferAvailable();

private:

    explicit SourceBufferPrivateHelio(MediaPlayerPrivateHelio *,
                                      MediaSourcePrivateHelio *,
                                      String);

    // During an append cycle, these methods would be called
    bool didDetectISOBMFFInitSegment(rcv_node_t *root);

    bool didDetectISOBMFFMediaSegment(rcv_node_t *root);

    WeakPtr<SourceBufferPrivateHelio> createWeakPtr() { return m_weakFactory.createWeakPtr(); }

    WeakPtrFactory<SourceBufferPrivateHelio> m_weakFactory;

    MediaPlayer::ReadyState m_readyState;

    SourceBufferPrivateClient* m_client;

    rcv_parser_t *m_rcvmfParser;
    MediaPlayerPrivateHelio *m_mediaPlayer;
    MediaSourcePrivateHelio *m_mediaSource;

    RefPtr<MediaDescriptionHelio> m_trackDescription;

    // Captured from the MVHD box when the init segment is appended,
    // then provided to each MediaSample so that it can calculate it's
    // duration, cts & dts.
    uint32_t m_timescale;

    bool m_writeBufferAvailable;

    // m_mediaStream is data in to be processed
    rcv_media_stream_t *m_mediaStream;
    // m_mediaPipeline is decoder resources and read off the m_mediaStream
    // and get synchronized using the clock.
    rcv_media_pipeline_t *m_mediaPipeline;

    bool m_isAudio;
    bool m_isVideo;
    bool m_encryptedSamples;

    // TODO: Do we need to keep this?
    rcv_node_t *m_isobmffInitSegmentRoot;

    RefPtr<HelioCodecConfiguration> m_codecConfiguration;
    // When an enqueued sample doesn't fit
    // in a buffer, it get's captured finishes
    // processing once additional space frees up.
    // TODO: Cange this to a RefPtr
    RefPtr<MediaSampleHelio> m_enqueuedSample;

    GenericTaskQueue<Timer> m_engineTaskQueue;

    // TODO: Capture a notify flag.
    bool m_notifyReadyForSample;

    AtomicString m_trackId;

};
}

#endif
