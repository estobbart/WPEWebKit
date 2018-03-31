#ifndef SourceBufferPrivateVHS_h
#define SourceBufferPrivateVHS_h

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include "SourceBufferPrivate.h"
#include "MediaPlatformPrivate.h"
#include "MediaDescription.h"
#include "GenericTaskQueue.h"

#include <vhs/vhs_isobmff.h>
#include <vhs/vhs_media_pipeline.h>

namespace WebCore {

class DataLoaderProxy;
class InitializationSegmentVHS;
class MediaSampleVHS;
class MediaPlatformPrivate;

class MediaDescriptionVHS final : public MediaDescription {
public:
    static RefPtr<MediaDescriptionVHS> create(const char *codec) { return adoptRef(new MediaDescriptionVHS(codec)); }
    virtual ~MediaDescriptionVHS() { }

    AtomicString codec() const override { return m_codec; }
    bool isVideo() const override { return m_isVideo; }
    bool isAudio() const override { return m_isAudio; }
    bool isText() const override { return m_isText; }

protected:
    MediaDescriptionVHS(const char *codec)
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

/**
 * There are two main phases to creating & setting up a SourceBufferPrivateVHS
 * with a libvhs media pipeline. The first is to setup the stream, which is the
 * input & format that the media pipeline should accept. The stream needs to be
 * provided callbacks to provide notifications back to the SourceBuffer when the
 * buffers become available. Setting up the stream should be done in the
 * initialization of the source buffer when the codec is supplied.
 * The second phase of setup is to create the decoder. This step occurs when
 * the initialization data is received from the application. The initialization
 * segment contains all the information needed to create a decoder.
 * Destroying a decoder and the stream need to occur in the removedFromMediaSource
 * method. This is due to the SourceBuffer being an ActiveDomObject, which
 * means it's desctructor doesn't  get called until garbage collection.
 * Resources that need to be free'd should be done in the removedFromMediaSource
 * method.
 * Even setClient(nullptr) doesn't occur until garbage collection. The
 * SourceBuffer knows that it has called removedFromMediaSource so additional
 * guards for `isRemoved` occur at that layer.
 */

class SourceBufferPrivateVHS final : public SourceBufferPrivate, public VHSMediaPipeline {

public:
    static RefPtr<SourceBufferPrivateVHS> create(const String&, Ref<DataLoaderProxy>&&, Ref<MediaPlatformPrivate>&&);
    virtual ~SourceBufferPrivateVHS();

    // SourceBufferPrivate method overrides required ---
    void setClient(SourceBufferPrivateClient*) override;

    void append(const unsigned char* data, unsigned length) override;
    void abort() override;
    void resetParserState() override;
    void removedFromMediaSource() override;

    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

    // SourceBufferPrivate method overrides optional (but not really) ---
    void flush(const AtomicString&) override;
    void enqueueSample(Ref<MediaSample>&&, const AtomicString&) override;
    // NOTE(estobbart): This gets called prior to finishing the append..
    // we only want to do this outside of the append cycle to keep append
    // times small.
    bool isReadyForMoreSamples(const AtomicString&) override;
    void notifyClientWhenReadyForMoreSamples(const AtomicString&) override;
    void setActive(bool) override;

    // SourceBufferPrivate method overrides ---

    // VHSMediaPipeline methods ---
    vhs_media_pipeline_t * mediaPipeline() const override { return m_vhsPipeline; }

    void notifyClientTask(const AtomicString&);

    void streamBufferAvailable();

    void writeSampleToStream(const AtomicString&);

private:
    SourceBufferPrivateVHS(const String&, Ref<DataLoaderProxy>&&, Ref<MediaPlatformPrivate>&&);

    RefPtr<MediaDescriptionVHS> createTrackDescription(vhs_node_t *trakNode) const;

    bool clientHasAudio() const;
    bool clientHasVideo() const;

    friend class DataLoaderProxy;

    SourceBufferPrivateClient* m_client { nullptr };

    Ref<DataLoaderProxy> m_dataLoaderProxy;
    Ref<MediaPlatformPrivate> m_mediaPlatformPrivate;

    vhs_parser_t* m_vhsParser;

    // NOTE(estobbart): see getter and setter method implementation for details
    // on this state.
    MediaPlayer::ReadyState m_state;

    RefPtr<InitializationSegmentVHS> m_initSegmentVHS;

    vhs_media_stream_t *m_vhsStream;
    vhs_media_pipeline_t *m_vhsPipeline;

    RefPtr<MediaSampleVHS> m_currentSample;

    bool m_flushed;

    // Keeps track if the buffer is currently appending or not. In an attempt
    // to keep append times really short, a buffer will return false for
    // isReadyForMoreSamples even if can enqueue the sample until after the
    // append reports done.
    bool m_appending;

    // m_notifyTaskQueue allows the SourceBufferPrivateVHS to notify the
    // SourceBufferClient that it's ready for more samples.
    // This is intentionally done when append is no longer on the stack
    // with that hope that the JS updateend event gets scheduled and fired
    // prior to accepting samples. Although this may slightly delay the start
    // time, it's worth it to keep consistent & short the updatestart-updateend
    // cycle.
    GenericTaskQueue<Timer> m_notifyTaskQueue;
    bool m_notifyWhenReadyForSamples;

    // m_bufferAvailable is initialized to false, first gets set to true by the
    // libvhs callback, and toggles as writes occur and space becomes available
    bool m_bufferAvailable;
};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)

#endif
