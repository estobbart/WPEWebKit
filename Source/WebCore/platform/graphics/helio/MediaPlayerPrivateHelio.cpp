
//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "MediaPlayerPrivateHelio.h"
#include "MediaSourcePrivateHelio.h"
#include "MediaSourcePrivateClient.h"
#include <wtf/NeverDestroyed.h>
#include <stdio.h>
//#include "helio.h"

//#if USE(TEXTURE_MAPPER_GL)
//#include "BitmapTextureGL.h"
//#include "BitmapTexturePool.h"
#include "TextureMapperGL.h"
//#endif
#if USE(COORDINATED_GRAPHICS_THREADED)
#include "TextureMapperPlatformLayerBuffer.h"
#endif

// Period timeupdate dispatcher
#include <pthread.h>
//#include <unistd.h>

namespace WebCore {

pthread_t time_update_thread_id;


static void *timeupdate_monitor(void *mediaPlayer) {
   while(true) {
       // printf("timeupdate_thread sleep 0.250\n");
       static_cast<MediaPlayer*>(mediaPlayer)->timeChanged();
       WTF::sleep(0.250);
   }
}

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
        // TODO: Get this from the librcvmf engine
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
    if (parameters.type == "image/svg+xml" || parameters.type == "application/xml"){
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
  if (!parameters.type.isEmpty()) {//  && parameters.type == "video/mp2ts") { // TODO:
      printf("MediaPlayerPrivateHelio::MediaPlayer::MayBeSupported !parameters.type.isEmpty()\n");
      // TODO: If we get a codec we can guarentee support, else it's maybe
      return MediaPlayer::IsNotSupported;
  }

  printf("MediaPlayerPrivateHelio::MediaPlayer::IsNotSupported\n");
  return MediaPlayer::IsNotSupported;
}

MediaPlayerPrivateHelio::MediaPlayerPrivateHelio(MediaPlayer* player)
    : m_mediaSourceClient()
    , m_rate(0)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_mediaPlayer(player)
    , m_mediaSourcePrivate(0) {
        printf("MediaPlayerPrivateHelio constructor\n");
#if USE(COORDINATED_GRAPHICS_THREADED)
        m_platformLayerProxy = adoptRef(new TextureMapperPlatformLayerProxy());
#endif
        
//#if USE(USE_HOLE_PUNCH_EXTERNAL)
#if USE(COORDINATED_GRAPHICS_THREADED)
        LockHolder locker(m_platformLayerProxy->lock());
        m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GraphicsContext3D::DONT_CARE));
#endif
//#endif
}

MediaPlayerPrivateHelio::~MediaPlayerPrivateHelio() {
    printf("MediaPlayerPrivateHelio destructor\n");
}

void MediaPlayerPrivateHelio::load(const String& url) {
    printf("MediaPlayerPrivateHelio load url%s\n", url.utf8().data());
}

void MediaPlayerPrivateHelio::load(const String& url, MediaSourcePrivateClient* client) {
    printf("MediaPlayerPrivateHelio load url:%s, client\n", url.utf8().data());
    m_mediaSourceClient = client;
    m_mediaSourcePrivate = MediaSourcePrivateHelio::create(this, m_mediaSourceClient);
    
    m_platformClockController = rcv_media_stream_clock_init();

    pthread_create(&time_update_thread_id, NULL, timeupdate_monitor, m_mediaPlayer);
}

void MediaPlayerPrivateHelio::cancelLoad() {
    printf("MediaPlayerPrivateHelio cancelLoad\n");
}

void MediaPlayerPrivateHelio::play() {
    printf("MediaPlayerPrivateHelio play\n");
    m_rate = 1;
}

void MediaPlayerPrivateHelio::pause() {
    printf("MediaPlayerPrivateHelio pause\n");
    m_rate = 0;
}

FloatSize MediaPlayerPrivateHelio::naturalSize() const {
    // TOOD: This happens a lot after HaveMetadata is reported..
    //printf("MediaPlayerPrivateHelio naturalSize 1280x720: TODO: hardcoded\n");
    return FloatSize(1280 /* width */, 720 /* height */);
}

// TODO: Need something better here
bool MediaPlayerPrivateHelio::hasVideo() const {
    //printf("MediaPlayerPrivateHelio hasVideo %i\n", m_readyState >= MediaPlayer::HaveMetadata);
    return m_readyState >= MediaPlayer::HaveMetadata;
}

// TODO: Need something better here
bool MediaPlayerPrivateHelio::hasAudio() const {
    //printf("MediaPlayerPrivateHelio hasAudio %i\n", m_readyState >= MediaPlayer::HaveMetadata);
    return m_readyState >= MediaPlayer::HaveMetadata;
}

// TODO: What does visible mean to us here?
void MediaPlayerPrivateHelio::setVisible(bool visible) {
    //printf("WARN MediaPlayerPrivateHelio setVisible:%i ignored\n", visible);
}
    
MediaTime MediaPlayerPrivateHelio::durationMediaTime() const {
    //printf("MediaPlayerPrivateHelio durationMediaTime\n");
    return m_mediaSourceClient->duration();
}

float MediaPlayerPrivateHelio::currentTime() const {
    rcv_media_platform_t *platform =  m_mediaSourcePrivate->mediaPlatform();
    if (!platform) {
       printf("MediaPlayerPrivateHelio::currentTime() NULL\n");
       return 0;
    }
    return rcv_media_platform_last_pts(platform) / 90000.0;
}

void MediaPlayerPrivateHelio::seekDouble(double time) {
    printf("MediaPlayerPrivateHelio seekDouble %f \n", time);
    // Notifying the timeChange dismisses that the player is in a seeking state
    setReadyState(MediaPlayer::HaveFutureData);
    // State must be > HaveFutureData to be able to dismiss the seeking??
    m_mediaPlayer->timeChanged();
}

bool MediaPlayerPrivateHelio::seeking() const {
    printf("MediaPlayerPrivateHelio seeking false\n");
    return false;
}

bool MediaPlayerPrivateHelio::paused() const {
    //printf("MediaPlayerPrivateHelio paused() m_rate == %u\n", m_rate);
    return m_rate == 0;
}

MediaPlayer::NetworkState MediaPlayerPrivateHelio::networkState() const {
    printf("WARN MediaPlayerPrivateHelio networkState may be empty!!!\n");
    // enum NetworkState { Empty, Idle, Loading, Loaded, FormatError, NetworkError, DecodeError };
    return m_readyState >= MediaPlayer::HaveMetadata ? MediaPlayer::Loaded : MediaPlayer::Empty;
}

// https://www.w3.org/TR/2011/WD-html5-20110405/video.html#the-ready-states
MediaPlayer::ReadyState MediaPlayerPrivateHelio::readyState() const {
    printf("MediaPlayerPrivateHelio readyState\n");
    // enum ReadyState  { HaveNothing, HaveMetadata, HaveCurrentData, HaveFutureData, HaveEnoughData };
    return m_readyState;
}

// TODO: It looks like this occurs and then dispatches the networkState change
// progress event. Check if setting this flag to false changes that event.
bool MediaPlayerPrivateHelio::didLoadingProgress() const {
    // TODO: Does this determine when "open" dispatches?
    // printf("MediaPlayerPrivateHelio didLoadingProgress\n");
    // this after being constructed this gets called repeatedly, if you return true, it calls visible and setSize
    return true;
}

void MediaPlayerPrivateHelio::setSize(const IntSize& size) {
    //printf("MediaPlayerPrivateHelio setSize\n");
    m_size = size;
}

void MediaPlayerPrivateHelio::paint(GraphicsContext& ctx, const FloatRect& rect) {
    printf("MediaPlayerPrivateHelio paint\n");
    ctx.clearRect(rect);
}

void MediaPlayerPrivateHelio::setNetworkState(MediaPlayer::NetworkState state __attribute__((unused))) {
/*    if (state == m_networkState)
        return;

    m_networkState = state;
    m_player->networkStateChanged(); */
    printf("ERROR: MediaPlayerPrivateHelio::setNetworkState not being captured!!!\n");
}

/**
 * a SourceBufferPrivate becomes active after having reported their initSegment
 * back the SourceBuffer. We use active SourceBuffers to trigger that we have
 * metadata.
 *
 * TODO: This may eventually mean we removed a source buffer also..
 */
void MediaPlayerPrivateHelio::mediaSourcePrivateActiveSourceBuffersChanged() {
    m_mediaPlayer->client().mediaPlayerActiveSourceBuffersChanged(m_mediaPlayer);
    setReadyState(MediaPlayer::HaveMetadata);
    // TODO: Fix this and point it somewhere more appropriate..
    m_mediaPlayer->networkStateChanged();
}
  
void MediaPlayerPrivateHelio::durationChanged() {
  m_mediaPlayer->durationChanged();
}

rcv_media_clock_controller_t * MediaPlayerPrivateHelio::platformClockController() {
    return m_platformClockController;
}

void MediaPlayerPrivateHelio::setReadyState(MediaPlayer::ReadyState stateChange) {
    // Need to print this and keep track of if anyone else is calling it..
    //if (stateChange == MediaPlayer::HaveMetadata && m_readyState < stateChange) {
    //    m_readyState = stateChange;
    //}
    m_readyState = stateChange;
    
    m_mediaPlayer->readyStateChanged();
}

}
