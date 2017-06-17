
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
        cache.get().add(ASCIILiteral("video/mp2ts"));
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
  printf("MediaPlayerPrivateHelio _supportsType:%s codec:%s url:%s keySystem:%s channels:%i isMediaSource:%i isMediaStream:%i\n",
      parameters.type.utf8().data(),
      parameters.codecs.utf8().data(),
      parameters.url.string().utf8().data(),
      parameters.keySystem.utf8().data(),
      parameters.channels,
      parameters.isMediaSource,
      parameters.isMediaStream);

  if (!parameters.type.isEmpty() && parameters.type == "video/mp2ts") {
      printf("MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported\n");
      // TODO: If we get a codec we can guarentee support, else it's maybe
      return MediaPlayer::MayBeSupported;
  }

  if (parameters.isMediaSource) {
    printf("MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported\n");
    return MediaPlayer::MayBeSupported;
  }

  printf("MediaPlayerPrivateHelio::MediaPlayer::IsNotSupported\n");
  return MediaPlayer::IsNotSupported;
}

MediaPlayerPrivateHelio::MediaPlayerPrivateHelio(MediaPlayer* player __attribute__((unused)))
  : m_mediaSourcePrivate(0) {
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

void MediaPlayerPrivateHelio::setNetworkState(MediaPlayer::NetworkState state __attribute__((unused)))
{
/*    if (state == m_networkState)
        return;

    m_networkState = state;
    m_player->networkStateChanged(); */
}

}
