
//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "MediaPlayerPrivateHelio.h"
#include "MediaSourcePrivateHelio.h"
#include <wtf/NeverDestroyed.h>
#include <stdio.h>
//#include "helio.h"

namespace WebCore {

/**
 * Called from MediaPlayer based on #defines
 */
void MediaPlayerPrivateHelio::registerMediaEngine(MediaEngineRegistrar registrar) {
    registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivateHelio>(player); }, // CreateMediaEnginePlayer
              _getSupportedTypes, // MediaEngineSupportedTypes
              _supportsType, // MediaEngineSupportsType
              0, // MediaEngineOriginsInMediaCache
              0, // MediaEngineClearMediaCache
              0, // MediaEngineClearMediaCacheForOrigins
              0); // MediaEngineSupportsKeySystem
}

// isTypeSupported
void MediaPlayerPrivateHelio::_getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& supportedTypes) {
    printf("MediaPlayerPrivateHelio _getSupportedTypes\n");
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cache;
    static bool isInitialized = false;

    if (!isInitialized) {
        isInitialized = true;
        // helio_enumerate_supported_mime_types(&cache, [](void *cache, const char *mime_type){
        //     cache->get().add(ASCIILiteral(mime_type));
        // });
        //cache.get().add(ASCIILiteral("video/mp2ts"));
        cache.get().add(ASCIILiteral("video/mp4"));
        cache.get().add(ASCIILiteral("audio/mp4"));
    }
    supportedTypes = cache;
}

/**
 * This gets called a handful of times when opening the inspector looking for
 * "image/svg+xml" support.
 * Once `MediaSource.isTypeSupported()` gets called it checks this function.
 * It's advised to report maybe until we know the codec.
 * (weird that it calls this three times)
 * It also gets called with "" when video.src is assigned.
 */
MediaPlayer::SupportsType MediaPlayerPrivateHelio::_supportsType(const MediaEngineSupportParameters& parameters) {
    if (parameters.type == "image/svg+xml"){
        return MediaPlayer::IsNotSupported;
    }

/*

MediaSource::isTypeSupported(video/mp4; codecs="avc1.4d4015")
MediaPlayerPrivateHelio _supportsType:video/mp4 codec:avc1.4d4015 url: keySystem: channels:0 isMediaSource:1 isMediaStream:0

MediaSource::isTypeSupported(video/mp4; codecs="avc1.4d401e")
MediaPlayerPrivateHelio _supportsType:video/mp4 codec:avc1.4d401e url: keySystem: channels:0 isMediaSource:1 isMediaStream:0
MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported parameters.isMediaSource

MediaSource::isTypeSupported(video/mp4; codecs="avc1.4d401f")
MediaPlayerPrivateHelio _supportsType:video/mp4 codec:avc1.4d401f url: keySystem: channels:0 isMediaSource:1 isMediaStream:0
MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported parameters.isMediaSource

MediaSource::isTypeSupported(audio/mp4; codecs="mp4a.40.5")
MediaPlayerPrivateHelio _supportsType:audio/mp4 codec:mp4a.40.5 url: keySystem: channels:0 isMediaSource:1 isMediaStream:0
MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported parameters.isMediaSource

*/
  printf("MediaPlayerPrivateHelio _supportsType:%s codec:%s url:%s keySystem:%s channels:%i isMediaSource:%i isMediaStream:%i\n",
      parameters.type.utf8().data(),
      parameters.codecs.utf8().data(),
      parameters.url.string().utf8().data(),
      parameters.keySystem.utf8().data(),
      parameters.channels,
      parameters.isMediaSource,
      parameters.isMediaStream);

  if (parameters.isMediaSource) {

      // TODO: Check these..
      if (!parameters.codecs.isEmpty()) {
          printf("MediaPlayerPrivateHelio::MediaPlayer::IsSupported parameters.isMediaSource and codecs\n");
          return MediaPlayer::IsSupported;
      }

      printf("MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported parameters.isMediaSource\n");
      return MediaPlayer::MayBeSupported;
  }

  // MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported !parameters.type.isEmpty()
  if (!parameters.type.isEmpty()){//  && parameters.type == "video/mp2ts") { // TODO:
      printf("MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported !parameters.type.isEmpty()\n");
      // TODO: If we get a codec we can guarentee support, else it's maybe
      return MediaPlayer::IsNotSupported;
  }

  printf("MediaPlayerPrivateHelio::MediaPlayer::IsNotSupported\n");
  return MediaPlayer::IsNotSupported;
}

MediaPlayerPrivateHelio::MediaPlayerPrivateHelio(MediaPlayer* player)
  : m_mediaPlayer(player)
  , m_mediaSourcePrivate(0) {
    //m_helioEngine = helio_init();
    printf("MediaPlayerPrivateHelio constructor\n");
}

MediaPlayerPrivateHelio::~MediaPlayerPrivateHelio() {
    printf("MediaPlayerPrivateHelio destructor\n");
}

void MediaPlayerPrivateHelio::load(const String& url) {
    printf("MediaPlayerPrivateHelio load url%s\n", url.utf8().data());
}

void MediaPlayerPrivateHelio::load(const String& url, MediaSourcePrivateClient* client) {
    printf("MediaPlayerPrivateHelio load url:%s, client\n", url.utf8().data());
    m_mediaSourcePrivate = MediaSourcePrivateHelio::create(this, client);
}

void MediaPlayerPrivateHelio::cancelLoad() {
    printf("MediaPlayerPrivateHelio cancelLoad\n");
}

void MediaPlayerPrivateHelio::play() {
    printf("MediaPlayerPrivateHelio play\n");
}

void MediaPlayerPrivateHelio::pause() {
    printf("MediaPlayerPrivateHelio pause\n");
}

FloatSize MediaPlayerPrivateHelio::naturalSize() const {
    printf("MediaPlayerPrivateHelio naturalSize\n");
    return FloatSize(0 /* width */, 0 /* height */);
}

bool MediaPlayerPrivateHelio::hasVideo() const {
    printf("MediaPlayerPrivateHelio hasVideo\n");
    return false;
}

bool MediaPlayerPrivateHelio::hasAudio() const {
    printf("MediaPlayerPrivateHelio hasAudio\n");
    return false;
}

void MediaPlayerPrivateHelio::setVisible(bool visible __attribute__((unused))) {
    //printf("MediaPlayerPrivateHelio setVisible:%i\n", visible);
}

bool MediaPlayerPrivateHelio::seeking() const {
    printf("MediaPlayerPrivateHelio seeking\n");
    return false;
}

bool MediaPlayerPrivateHelio::paused() const {
    printf("MediaPlayerPrivateHelio paused\n");
    return false;
}

MediaPlayer::NetworkState MediaPlayerPrivateHelio::networkState() const {
    printf("MediaPlayerPrivateHelio networkState\n");
    // enum NetworkState { Empty, Idle, Loading, Loaded, FormatError, NetworkError, DecodeError };
    return MediaPlayer::Empty;
}

MediaPlayer::ReadyState MediaPlayerPrivateHelio::readyState() const {
    printf("MediaPlayerPrivateHelio readyState\n");
    // enum ReadyState  { HaveNothing, HaveMetadata, HaveCurrentData, HaveFutureData, HaveEnoughData };
    return MediaPlayer::HaveNothing;
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateHelio::buffered() const {
    printf("MediaPlayerPrivateHelio buffered\n");
    auto timeRanges = std::make_unique<PlatformTimeRanges>();
    MediaTime rangeStart = MediaTime::zeroTime();
    MediaTime rangeEnd = MediaTime::zeroTime();
    timeRanges->add(rangeStart, rangeEnd);
    return timeRanges;
}

bool MediaPlayerPrivateHelio::didLoadingProgress() const {
    // TODO: Does this determine when "open" dispatches?
    // printf("MediaPlayerPrivateHelio didLoadingProgress\n");
    // this after being constructed this gets called repeatedly, if you return true, it calls visible and setSize
    return true;
}

void MediaPlayerPrivateHelio::setSize(const IntSize& size __attribute__((unused))) {
    //printf("MediaPlayerPrivateHelio setSize\n");
}

void MediaPlayerPrivateHelio::paint(GraphicsContext& ctx __attribute__((unused)), const FloatRect& rect __attribute__((unused))) {
    //printf("MediaPlayerPrivateHelio paint\n");
}

void MediaPlayerPrivateHelio::setNetworkState(MediaPlayer::NetworkState state __attribute__((unused))) {
/*    if (state == m_networkState)
        return;

    m_networkState = state;
    m_player->networkStateChanged(); */
}

void MediaPlayerPrivateHelio::mediaSourcePrivateActiveSourceBuffersChanged() {
  m_mediaPlayer->client().mediaPlayerActiveSourceBuffersChanged(m_mediaPlayer);
}

}
