
#include "config.h"
#include "MediaSourcePrivateVHS.h"

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include <stdio.h>

#include "ContentType.h"
#include "SourceBufferPrivateVHS.h"
#include "DataLoaderProxy.h"
#include "ReadyStateResponder.h"
#include "MediaPlatformPrivate.h"
// client calls happen in the MediaPlayerPrivate currently
// #include "MediaSourcePrivateClient.h"

namespace WebCore {

// NOTE(estobbart): may make this a unique pointer..
class MediaPlatformPrivateVHS final : public MediaPlatformPrivate {
public:
    static RefPtr<MediaPlatformPrivateVHS> create() {
        return adoptRef(new MediaPlatformPrivateVHS());
    }
    virtual ~MediaPlatformPrivateVHS() {
        if (m_vhsPlatform) {
            vhs_media_platform_destroy(&m_vhsPlatform);
        }
    }

    void connectPipeline(VHSMediaPipeline *pipeline) const {
        printf("%s %p\n", __PRETTY_FUNCTION__);
        printf("mediaPipeline(%p)\n", pipeline->mediaPipeline());
        vhs_media_platform_connect(m_vhsPlatform, pipeline->mediaPipeline());

        // TODO: needs to happen in the MediaPlayerPrivate
        // TODO: move 'play' somewhere else..
        static uint8_t count = 0;
        count++;
        if (count == 2) {
            printf("vhs_media_platform_play\n");
            vhs_media_platform_play(m_vhsPlatform);
        }
    }

    uint64_t lastTimeStamp() const {
        return vhs_media_platform_last_pts(m_vhsPlatform);
    }

private:
    MediaPlatformPrivateVHS() {
        // TODO: A 'configured' callback or some sort of 'ready' callback
        m_vhsPlatform = vhs_media_platform_init();
    }

    vhs_media_platform_t *m_vhsPlatform;
};

/**
 * NOTE(estobbart): This flow causes sourceopen to fire after loadstart
 */
Ref<MediaSourcePrivateVHS> MediaSourcePrivateVHS::create(MediaClockReporter* clockReporter, ReadyStateResponder* readyStateResponder) {
    // printf("MediaSourcePrivateVHS create\n");
    // RefPtr<MediaSourcePrivateVHS> mediaSourcePrivate = adoptRef(new MediaSourcePrivateVHS(clockReporter));
    // printf("... client->setPrivateAndOpen\n");
    // // TODO: fix the ref count churn.. setPrivateAndOpen(Ref<MediaSourcePrivate>&&)
    // // setPrivateAndOpen takes ownership
    // client->setPrivateAndOpen(WTFMove(mediaSourcePrivate));
    // clockReporter->activateReporter(mediaSourcePrivate.get());
    //return mediaSourcePrivate;
    return adoptRef(*new MediaSourcePrivateVHS(clockReporter, readyStateResponder));
}

/**
 * MediaSourcePrivateVHS does not take ownership of the clockReporter,
 *
 */
MediaSourcePrivateVHS::MediaSourcePrivateVHS(MediaClockReporter* clockReporter, ReadyStateResponder* readyStateResponder)
  : m_clockReporter(clockReporter)
  , m_readyStateResponder(readyStateResponder) {
    printf("MediaSourcePrivateVHS constructor\n");
    m_dataLoaderProxy = DataLoaderProxy::create(m_readyStateResponder);
    m_vhsMediaPlatform = MediaPlatformPrivateVHS::create();
    m_clockReporter->activateReporter(this);
    // TODO: Clock this..
    //m_mediaPlatform = rcv_media_platform_init();
}

MediaSourcePrivateVHS::~MediaSourcePrivateVHS() {
    printf("*******    MediaSourcePrivateVHS destructor    *******\n");
    // if (m_mediaPlatform) {
    //     rcv_media_platform_destroy(&m_mediaPlatform);
    // }
    m_clockReporter->deactivateReporter();
}

/**
 * https://www.w3.org/TR/media-source/#dom-mediasource-addsourcebuffer
 */
MediaSourcePrivate::AddStatus MediaSourcePrivateVHS::addSourceBuffer(const ContentType& contentType,
                                                                     RefPtr<SourceBufferPrivate>& sourceBufferPrivate) {
  // enum AddStatus { Ok, NotSupported, ReachedIdLimit };

  // This could be pased to the engine to init a pipeline..

  // H.264 Baseline: avc1.42E0xx, where xx is the AVC level
  // H.264 Main: avc1.4D40xx, where xx is the AVC level
  // H.264 High: avc1.6400xx, where xx is the AVC level

  // // We currently only support single codec per buffer
  // if (contentType.codecs().size() != 1) {
  //     return MediaSourcePrivate::NotSupported;
  // }
  //
  // String firstCodec = contentType.codecs().first();
  printf("MediaSourcePrivateHelio addSourceBuffer contentType:%s\n", contentType.raw().utf8().data());

  // // TODO: Would mean exposing _supportsType from MediaPlayerPrivateHelio
  // // Doesn't really make much sense to check again here unless we got the codec
  // // Our MediaPlayer has already been determined by the engine to be best suited
  // // as supporting MediaSource.
  // // TODO: Add a static SourceBuffer method to check this. probbaly needs a decoder
  // // handle to be able to it, so may require a instance.
  // if (firstCodec.startsWithIgnoringASCIICase(String("mp4"))
  //     || firstCodec.startsWithIgnoringASCIICase(String("avc1"))) {
  //     // TODO: We know the content type here.. so we should pass that along.
  //     sourceBufferPrivate = SourceBufferPrivateHelio::create(m_mediaPlayer,
  //                                                            this,
  //                                                            firstCodec);
  //     m_pendingBufferActivation++;
  //     return MediaSourcePrivate::Ok;
  // }
  //return MediaSourcePrivate::NotSupported;
  RefPtr<SourceBufferPrivateVHS> sourceBuffer = SourceBufferPrivateVHS::create(contentType.parameter(contentType.codecsParameter()),
                                                                               *m_dataLoaderProxy,
                                                                               *m_vhsMediaPlatform);
  m_dataLoaderProxy->addSourceBuffer(sourceBuffer.copyRef());
  sourceBufferPrivate = sourceBuffer;

  return MediaSourcePrivate::Ok;
}

// See notes in the header file..
void MediaSourcePrivateVHS::removeSourceBuffer(SourceBufferPrivate* sourceBufferPrivate __attribute__((unused))) {
    printf("ERROR MediaSourcePrivateVHS removeSourceBuffer\n");
}

/**
 * https://www.w3.org/TR/media-source/#duration-change-algorithm
 * See MSE_VHS.md on the details of the durationChange flow.
 */
void MediaSourcePrivateVHS::durationChanged() {
    printf("MediaSourcePrivateVHS durationChanged\n");
    m_clockReporter->durationChanged();
}

/**
 * https://www.w3.org/TR/media-source/#end-of-stream-algorithm
 */
void MediaSourcePrivateVHS::markEndOfStream(EndOfStreamStatus status) {
    printf("ERROR MediaSourcePrivateVHS markEndOfStream:%i\n", status);
}

void MediaSourcePrivateVHS::unmarkEndOfStream() {
    printf("ERROR MediaSourcePrivateVHS unmarkEndOfStream\n");
}

MediaPlayer::ReadyState MediaSourcePrivateVHS::readyState() const {
    // printf("MediaSourcePrivateVHS readyState\n");
    m_readyStateResponder->readyState();
}

void MediaSourcePrivateVHS::setReadyState(MediaPlayer::ReadyState readyState) {
    // printf("MediaSourcePrivateVHS setReadyState:%i\n", readyState);
    m_readyStateResponder->setReadyState(readyState);

}

/**
 * Called when the time requested is not within the buffered range
 * TODO: Not currently supported, it is assumed that all seek requests
 * are within the buffered range
 */
void MediaSourcePrivateVHS::waitForSeekCompleted() {
    printf("ERROR TODO: MediaSourcePrivateVHS waitForSeekCompleted\n");
}

// TODO: Test seek operations and the sequence of events that dispatch..
void MediaSourcePrivateVHS::seekCompleted() {
    printf("MediaSourcePrivateVHS seekCompleted\n");
    // We may need to write a buffer before the timechange occurs
    // m_clockReporter->reportTime();
    m_clockReporter->setSeeking(false);
}

float MediaSourcePrivateVHS::lastPresentedTimestamp() {
    printf("MediaSourcePrivateVHS lastPresentedTimestamp\n");
    float result = 0.0;
    if (m_vhsMediaPlatform) {
        result = m_vhsMediaPlatform->lastTimeStamp() / 90000.0;
    }
    printf("MediaSourcePrivateVHS lastPresentedTimestamp:%f\n", result);
    return result;
}

// rcv_media_platform_t * MediaSourcePrivateHelio::mediaPlatform() {
//     return m_mediaPlatform;
// }

}

#endif
