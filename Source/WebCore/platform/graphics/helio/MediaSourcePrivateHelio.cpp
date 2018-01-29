//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include <stdio.h>

#include "ContentType.h"
#include "MediaSourcePrivateClient.h"

#include "MediaSourcePrivateHelio.h"
#include "SourceBufferPrivateHelio.h"
#include "MediaPlayerPrivateHelio.h"

#define MSPH_PRINT 0

namespace WebCore {

RefPtr<MediaSourcePrivateHelio> MediaSourcePrivateHelio::create(MediaPlayerPrivateHelio* parent,
                                                                MediaSourcePrivateClient* client) {
// #if MSPH_PRINT
    printf("MediaSourcePrivateHelio create\n");
// #endif
    RefPtr<MediaSourcePrivateHelio> mediaSourcePrivate = adoptRef(new MediaSourcePrivateHelio(parent, client));
#if MSPH_PRINT
    printf("... client->setPrivateAndOpen\n");
#endif
    client->setPrivateAndOpen(*mediaSourcePrivate);
    return mediaSourcePrivate;
}

MediaSourcePrivateHelio::MediaSourcePrivateHelio(MediaPlayerPrivateHelio* helioPlayer, MediaSourcePrivateClient*)
  : m_mediaPlayer(helioPlayer)
  , m_pendingBufferActivation(0)
    // TODO: IS this the right ready state?
  , m_readyState(MediaPlayer::HaveNothing) {
#if MSPH_PRINT
    printf("MediaSourcePrivateHelio constructor\n");
#endif
    // TODO: Clock this..
    m_mediaPlatform = rcv_media_platform_init();
}

MediaSourcePrivateHelio::~MediaSourcePrivateHelio() {
// #if MSPH_PRINT
    printf("*******    MediaSourcePrivateHelio destructor    *******\n");
// #endif
    if (m_mediaPlatform) {
        rcv_media_platform_destroy(&m_mediaPlatform);
    }
}

/**
 * https://www.w3.org/TR/media-source/#dom-mediasource-addsourcebuffer
 */
MediaSourcePrivate::AddStatus MediaSourcePrivateHelio::addSourceBuffer(const ContentType& contentType,
                                                                       RefPtr<SourceBufferPrivate>& sourceBufferPrivate) {
  // enum AddStatus { Ok, NotSupported, ReachedIdLimit };

  // This could be pased to the helio engine to init a pipeline..

// H.264 Baseline: avc1.42E0xx, where xx is the AVC level
// H.264 Main: avc1.4D40xx, where xx is the AVC level
// H.264 High: avc1.6400xx, where xx is the AVC level
/**
class ContentType {
public:
    explicit ContentType(const String& type);

    String parameter(const String& parameterName) const;
    String type() const;
    Vector<String> codecs() const;
    const String& raw() const { return m_type; }
private:
    String m_type;
};
*/

  // We currently only support single codec per buffer
  if (contentType.codecs().size() != 1) {
      return MediaSourcePrivate::NotSupported;
  }

  String firstCodec = contentType.codecs().first();
#if MSPH_PRINT
  printf("MediaSourcePrivateHelio addSourceBuffer contentType:%s codec:%s\n",
         contentType.type().utf8().data(), firstCodec.utf8().data());
#endif

  // TODO: Would mean exposing _supportsType from MediaPlayerPrivateHelio
  // Doesn't really make much sense to check again here unless we got the codec
  // Our MediaPlayer has already been determined by the engine to be best suited
  // as supporting MediaSource.
  // TODO: Add a static SourceBuffer method to check this. probbaly needs a decoder
  // handle to be able to it, so may require a instance.
  if (firstCodec.startsWithIgnoringASCIICase(String("mp4"))
      || firstCodec.startsWithIgnoringASCIICase(String("avc1"))) {
      // TODO: We know the content type here.. so we should pass that along.
      sourceBufferPrivate = SourceBufferPrivateHelio::create(m_mediaPlayer,
                                                             this,
                                                             firstCodec);
      m_pendingBufferActivation++;
      return MediaSourcePrivate::Ok;
  }

  return MediaSourcePrivate::NotSupported;
}

// See notes in the header file..
void MediaSourcePrivateHelio::removeSourceBuffer(SourceBufferPrivate* sourceBufferPrivate __attribute__((unused))) {
    printf("ERROR MediaSourcePrivateHelio removeSourceBuffer\n");
}

/**
 * https://www.w3.org/TR/media-source/#duration-change-algorithm
 */
void MediaSourcePrivateHelio::durationChanged() {
#if MSPH_PRINT
    printf("MediaSourcePrivateHelio durationChanged\n");
#endif
    m_mediaPlayer->durationChanged();
}

/**
 * https://www.w3.org/TR/media-source/#end-of-stream-algorithm
 */
void MediaSourcePrivateHelio::markEndOfStream(EndOfStreamStatus status) {
    printf("ERROR MediaSourcePrivateHelio markEndOfStream:%i\n", status);
}

void MediaSourcePrivateHelio::unmarkEndOfStream() {
    printf("ERROR MediaSourcePrivateHelio unmarkEndOfStream\n");
}

/**
 * https://www.w3.org/TR/media-source/#dom-readystate
 * Indicates the current state of the MediaSource object. When the MediaSource
 * is created readyState must be set to "closed".
 */
//
MediaPlayer::ReadyState MediaSourcePrivateHelio::readyState() const {
#if MSPH_PRINT
    printf("MediaSourcePrivateHelio readyState\n");
#endif
    return m_readyState;
}

// TODO: Does this call closed??? We could free resources here..
//     enum ReadyState  { HaveNothing, HaveMetadata, HaveCurrentData, HaveFutureData, HaveEnoughData };
void MediaSourcePrivateHelio::setReadyState(MediaPlayer::ReadyState readyState) {
#if MSPH_PRINT
    printf("MediaSourcePrivateHelio setReadyState:%i\n", readyState);
#endif
    m_readyState = readyState;
}

void MediaSourcePrivateHelio::waitForSeekCompleted() {
    printf("ERROR MediaSourcePrivateHelio waitForSeekCompleted\n");
}

void MediaSourcePrivateHelio::seekCompleted() {
    printf("ERROR MediaSourcePrivateHelio seekCompleted\n");
}

void MediaSourcePrivateHelio::sourceBufferPrivateActiveStateChanged() {
#if MSPH_PRINT
    printf("MediaSourcePrivateHelio sourceBufferPrivateActiveStateChanged\n");
#endif
    m_pendingBufferActivation--;
    if (m_pendingBufferActivation == 0) {
        rcv_media_platform_connect_pipelines(m_mediaPlatform);
    }

    // m_sourceBuffers.set(sourceBuffer, mediaPipeline);
    //
    // // TODO: This can take up ~100ms, especially when calling startStream
    // bool allCreatedAreActive = true;
    // rcv_media_pipeline_t **activePipelines = malloc(sizeof(rcv_media_pipeline_t *) * m_sourceBuffers.size());
    //
    // uint8_t idx = 0;
    // for (rcv_media_pipeline_t *pipeline : m_sourceBuffers.values()) {
    //     if (!pipeline) {
    //         allCreatedAreActive = false;
    //         break;
    //     }
    //     activePipelines[idx++] = pipeline;
    // }
    //
    // activePipelines[m_sourceBuffers.size()] = NULL;
    //
    // if (allCreatedAreActive) {
    //
    //     // callOnMainThread...
    //
    //     rcv_media_platform_connect_pipelines(m_mediaPlatform, activePipelines);
    //     printf("MediaSourcePrivateHelio .. rcv_media_platform_connect_pipelines\n");
    //     for (auto sourceBuffer : m_sourceBuffers.keys()) {
    //         sourceBuffer->startStream();
    //     }
    //     printf("MediaSourcePrivateHelio .. startStream DONE\n");
    //
    // }
    //
    //
    // free(activePipelines);
    // TODO: Is this really the responsiblity of the MediaPlayer?
    m_mediaPlayer->mediaSourcePrivateActiveSourceBuffersChanged();
}

rcv_media_platform_t * MediaSourcePrivateHelio::mediaPlatform() {
    return m_mediaPlatform;
}

}
