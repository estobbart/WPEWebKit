//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include <stdio.h>

#include "ContentType.h"
#include "MediaSourcePrivateClient.h"

#include "MediaSourcePrivateHelio.h"
#include "SourceBufferPrivateHelio.h"
#include "MediaPlayerPrivateHelio.h"

namespace WebCore {

RefPtr<MediaSourcePrivateHelio> MediaSourcePrivateHelio::create(MediaPlayerPrivateHelio* parent,
                                                                MediaSourcePrivateClient* client) {
    printf("MediaSourcePrivateHelio create\n");
    RefPtr<MediaSourcePrivateHelio> mediaSourcePrivate = adoptRef(new MediaSourcePrivateHelio(parent, client));
    printf("... client->setPrivateAndOpen\n");
    client->setPrivateAndOpen(*mediaSourcePrivate);
    return mediaSourcePrivate;
}

MediaSourcePrivateHelio::MediaSourcePrivateHelio(MediaPlayerPrivateHelio* helioPlayer, MediaSourcePrivateClient*)
  : m_mediaPlayer(helioPlayer)
    // TODO: IS this the right ready state?
  , m_readyState(MediaPlayer::HaveNothing) {
    printf("MediaSourcePrivateHelio constructor\n");
      
   m_mediaPlatform = rcv_media_platform_init();
}

MediaSourcePrivateHelio::~MediaSourcePrivateHelio() {
    printf("MediaSourcePrivateHelio destructor\n");
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

  String firstCodec = contentType.codecs().first();
  printf("MediaSourcePrivateHelio addSourceBuffer contentType:%s\n", contentType.type().utf8().data());
  // TODO: Would mean exposing _supportsType from MediaPlayerPrivateHelio
  // Doesn't really make much sense to check again here unless we got the codec
  // Our MediaPlayer has already been determined by the engine to be best suited
  // as supporting MediaSource. TODO: If we return NotSupported, what happens?
  //
  // MediaEngineSupportParameters parameters;
  // parameters.isMediaSource = true;
  // parameters.type = contentType.type();
  // parameters.codecs = contentType.parameter(ASCIILiteral("codecs"));
  // if (MediaPlayerPrivateHelio::supportsType(parameters) == MediaPlayer::IsNotSupported)
  //     return MediaSourcePrivate::NotSupported;

  // if (m_sourceBuffers.size()) {
  //     // TODO: Until we add MP4 support
  //     return MediaSourcePrivate::ReachedIdLimit;
  // }

  // TODO: We know the content type here.. so we should pass that along.
  sourceBufferPrivate = SourceBufferPrivateHelio::create(m_mediaPlayer, this, firstCodec);
  m_sourceBuffers.add(sourceBufferPrivate, NULL);

  return MediaSourcePrivate::Ok;
}

void MediaSourcePrivateHelio::removeSourceBuffer(SourceBufferPrivate* sourceBufferPrivate __attribute__((unused))) {
    printf("MediaSourcePrivateHelio removeSourceBuffer\n");
}

/**
 * https://www.w3.org/TR/media-source/#duration-change-algorithm
 */
void MediaSourcePrivateHelio::durationChanged() {
    printf("MediaSourcePrivateHelio durationChanged\n");
    m_mediaPlayer->durationChanged();
}

/**
 * https://www.w3.org/TR/media-source/#end-of-stream-algorithm
 */
void MediaSourcePrivateHelio::markEndOfStream(EndOfStreamStatus status) {
    printf("MediaSourcePrivateHelio markEndOfStream:%i\n", status);
}

void MediaSourcePrivateHelio::unmarkEndOfStream() {
    printf("MediaSourcePrivateHelio unmarkEndOfStream\n");
}

/**
 * https://www.w3.org/TR/media-source/#dom-readystate
 * Indicates the current state of the MediaSource object. When the MediaSource
 * is created readyState must be set to "closed".
 */
// 
MediaPlayer::ReadyState MediaSourcePrivateHelio::readyState() const {
    printf("MediaSourcePrivateHelio readyState\n");
    return m_readyState;
}

void MediaSourcePrivateHelio::setReadyState(MediaPlayer::ReadyState readyState) {
    printf("MediaSourcePrivateHelio setReadyState:%i\n", readyState);
    m_readyState = readyState;
}

void MediaSourcePrivateHelio::waitForSeekCompleted() {
    printf("MediaSourcePrivateHelio waitForSeekCompleted\n");
}

void MediaSourcePrivateHelio::seekCompleted() {
    printf("MediaSourcePrivateHelio seekCompleted\n");
}

void MediaSourcePrivateHelio::sourceBufferPrivateActiveStateChanged(SourceBufferPrivateHelio *sourceBuffer,
                                                                    rcv_media_pipeline_t *mediaPipeline) {
    printf("MediaSourcePrivateHelio sourceBufferPrivateActiveStateChanged\n");
    
    m_sourceBuffers.set(sourceBuffer, mediaPipeline);

    // TODO: This can take up ~100ms, especially when calling startStream
    bool allCreatedAreActive = true;
    rcv_media_pipeline_t **activePipelines = malloc(sizeof(rcv_media_pipeline_t *) * m_sourceBuffers.size());

    uint8_t idx = 0;
    for (rcv_media_pipeline_t *pipeline : m_sourceBuffers.values()) {
        if (!pipeline) {
            allCreatedAreActive = false;
            break;
        }
        activePipelines[idx++] = pipeline;
    }

    activePipelines[m_sourceBuffers.size()] = NULL;
    
    if (allCreatedAreActive) {
        
        // callOnMainThread...
        
        rcv_media_platform_connect_pipelines(m_mediaPlatform, activePipelines);
        printf("MediaSourcePrivateHelio .. rcv_media_platform_connect_pipelines\n");
        for (auto sourceBuffer : m_sourceBuffers.keys()) {
            sourceBuffer->startStream();
        }
        printf("MediaSourcePrivateHelio .. startStream DONE\n");

    }


    free(activePipelines);
    m_mediaPlayer->mediaSourcePrivateActiveSourceBuffersChanged();
}

rcv_media_platform_t * MediaSourcePrivateHelio::mediaPlatform() {
    return m_mediaPlatform;
}

}
