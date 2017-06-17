//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include <stdio.h>

#include "ContentType.h"
#include "MediaSourcePrivateClient.h"

#include "MediaSourcePrivateHelio.h"
#include "SourceBufferPrivateHelio.h"

namespace WebCore {

RefPtr<MediaSourcePrivateHelio> MediaSourcePrivateHelio::create(MediaPlayerPrivateHelio* parent,
                                                                MediaSourcePrivateClient* client) {
    printf("MediaSourcePrivateHelio create\n");
    RefPtr<MediaSourcePrivateHelio> mediaSourcePrivate = adoptRef(new MediaSourcePrivateHelio(parent, client));
    client->setPrivateAndOpen(*mediaSourcePrivate);
    return mediaSourcePrivate;
}

MediaSourcePrivateHelio::MediaSourcePrivateHelio(MediaPlayerPrivateHelio*, MediaSourcePrivateClient*)
  : m_readyState(MediaPlayer::HaveNothing) {
    printf("MediaSourcePrivateHelio constructor\n");
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
  if (m_sourceBuffers.size()) {
      // TODO: Until we add MP4 support
      return MediaSourcePrivate::ReachedIdLimit;
  }

  m_sourceBuffers.append(SourceBufferPrivateHelio::create(this));
  sourceBufferPrivate = m_sourceBuffers.last();

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

}
