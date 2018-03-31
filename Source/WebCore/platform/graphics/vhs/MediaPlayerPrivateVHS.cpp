
//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"
#include "MediaPlayerPrivateVHS.h"

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include "GraphicsContext.h" // clearRect
#include "MediaClockReporter.h"
#include "MediaSourcePrivateVHS.h"
#include "MediaSourcePrivateClient.h"
#include "DataLoaderProxy.h"
#include <wtf/NeverDestroyed.h>
#include <stdio.h>
#include <cmath>

// #if USE(COORDINATED_GRAPHICS_THREADED)
// #include "TextureMapperGL.h"
// #include "TextureMapperPlatformLayerBuffer.h"
// #endif

namespace WebCore {

/**
 * Called from MediaPlayer based on #defines
 */
void MediaPlayerPrivateVHS::registerMediaEngine(MediaEngineRegistrar registrar) {
    registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivateVHS>(player); }, // CreateMediaEnginePlayer
              _getSupportedTypes, // MediaEngineSupportedTypes
              _supportsType, // MediaEngineSupportsType
              0, // MediaEngineOriginsInMediaCache
              0, // MediaEngineClearMediaCache
              0, // MediaEngineClearMediaCacheForOrigins
              _supportsKeySystem); // MediaEngineSupportsKeySystem
}

// isTypeSupported
void MediaPlayerPrivateVHS::_getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& supportedTypes) {
    printf("MediaPlayerPrivateVHS _getSupportedTypes\n");
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cache;
    static bool isInitialized = false;

    if (!isInitialized) {
        isInitialized = true;
        //cache.get().add(ASCIILiteral("video/mp2ts"));
        // TODO: This need to be based on whatever libvhs byte stream
        // parsing is supported.
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
MediaPlayer::SupportsType MediaPlayerPrivateVHS::_supportsType(const MediaEngineSupportParameters& parameters) {
    printf("MediaPlayerPrivateVHS::_supportsType: %s", parameters.type.raw().utf8().data());
  //   if (parameters.type.raw() == "image/svg+xml"){
  //       return MediaPlayer::IsNotSupported;
  //   }
  // printf("MediaPlayerPrivateVHS _supportsType:%s codec:%s url:%s keySystem:%s channels:%i isMediaSource:%i isMediaStream:%i\n",
  //     parameters.type.raw().utf8().data(),
  //     parameters.codecs.utf8().data(),
  //     parameters.url.string().utf8().data(),
  //     parameters.keySystem.utf8().data(),
  //     parameters.channels,
  //     parameters.isMediaSource,
  //     parameters.isMediaStream);
  //
  // if (!parameters.type.isEmpty()){//  && parameters.type == "video/mp2ts") { // TODO:
  //     printf("MediaPlayerPrivateVHS::MediaPlayer::MayBeSupported\n");
  //     // TODO: If we get a codec we can guarentee support, else it's maybe
  //     return MediaPlayer::MayBeSupported;
  // }
  //
  /**
   * TODO: We'll need to add two levels to the inner codec check,
   * first does the hardward support it. second, can libvhs convert it from the
   * container type.
   *
   * https://www.w3.org/2013/12/byte-stream-format-registry/
   * http://www.iana.org/assignments/media-types/media-types.xhtml
   * http://www.rfc-editor.org/rfc/rfc4337.txt
   * https://tools.ietf.org/html/rfc6381
   * https://tools.ietf.org/html/rfc3551
   */
  if (parameters.isMediaSource) {
      printf(" containerType: %s", parameters.type.containerType().utf8().data());
      // NOTE(estobbart): parameters.type.codecs() returns full codec strings
      // including profile & level
      if (!parameters.type.codecsParameter().isEmpty()) {
          if (parameters.type.containerType() == "video/mp4") {
              for (auto& codec : parameters.type.codecs()){
                  printf(" codec: %s", codec.utf8().data());
                  if (!codec.startsWithIgnoringASCIICase("avc1")) {
                      printf(" parameters.isMediaSource -> MediaPlayer::IsNotSupported\n");
                      return MediaPlayer::IsNotSupported;
                  }
              }
              printf(" parameters.isMediaSource -> MediaPlayer::IsSupported\n");
              return MediaPlayer::IsSupported;
          } else if (parameters.type.containerType() == "audio/mp4") {
              for (auto& codec : parameters.type.codecs()){
                  printf(" codec: %s", codec.utf8().data());
                  if (!codec.startsWithIgnoringASCIICase("mp4a")) {
                      printf(" parameters.isMediaSource -> MediaPlayer::IsNotSupported\n");
                      return MediaPlayer::IsNotSupported;
                  }
              }
              printf(" parameters.isMediaSource -> MediaPlayer::IsSupported\n");
              return MediaPlayer::IsSupported;
          }
          // Codec exists, but container type didn't match up
          printf(" MediaPlayer::IsNotSupported\n");
          return MediaPlayer::IsNotSupported;
      }
      // No codec, but is MediaSource. May want an additional containerType
      // check here..
      printf(" parameters.isMediaSource -> MediaPlayer::MayBeSupported\n");
      return MediaPlayer::MayBeSupported;
  }
  // Is not MediaSource, not supported.
  printf(" MediaPlayer::IsNotSupported\n");
  return MediaPlayer::IsNotSupported;
}

// TODO: VHS does not currently support any keySystems, but will eventually need
// to wire this up for clear key
bool MediaPlayerPrivateVHS::_supportsKeySystem(const String& keySystem,
                                               const String& mimeType) {
    printf("TODO: MediaPlayerPrivateVHS::_supportsKeySystem %s, %s;",
           keySystem.utf8().data(),
           mimeType.utf8().data());
    return false;
}

MediaPlayerPrivateVHS::MediaPlayerPrivateVHS(MediaPlayer* player)
    : m_player(player)
    , m_paused(true)
    , m_size()
    , m_punchRect()
    , m_networkState(MediaPlayer::Empty)
    , m_dataLoaderProxy(nullptr)
    , m_readyStateResponder(ReadyStateResponder::create(this)) {
    printf("MediaPlayerPrivateVHS constructor\n");
// #if USE(COORDINATED_GRAPHICS_THREADED)
//     m_platformLayerProxy = adoptRef(new TextureMapperPlatformLayerProxy());
//     LockHolder locker(m_platformLayerProxy->lock());
//     m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GraphicsContext3D::DONT_CARE));
// #endif
}

MediaPlayerPrivateVHS::~MediaPlayerPrivateVHS() {
    printf("MediaPlayerPrivateVHS destructor\n");
}

void MediaPlayerPrivateVHS::load(const String& url) {
    printf("ERROR: MediaPlayerPrivateVHS load url%s no client\n", url.utf8().data());
    m_networkState = MediaPlayer::FormatError;
    m_player->networkStateChanged();
}

/**
 * VHS only supports the MediaSourceExtension API's
 */
void MediaPlayerPrivateVHS::load(const String& url, MediaSourcePrivateClient* client) {
    printf("MediaPlayerPrivateVHS load url:%s, client\n", url.utf8().data());
    if (!m_clockReporter) {
        m_clockReporter = MediaClockReporter::create(m_player);
    }
    m_sourceClient = client;
    Ref<MediaSourcePrivateVHS> mediaSourcePrivate = MediaSourcePrivateVHS::create(m_clockReporter.get(), m_readyStateResponder.get());
    m_dataLoaderProxy = mediaSourcePrivate->dataLoaderProxy();
    m_sourceClient->setPrivateAndOpen(WTFMove(mediaSourcePrivate));
}

#if ENABLE(MEDIA_STREAM)
void MediaPlayerPrivateVHS::load(MediaStreamPrivate&) override {
    m_networkState = MediaPlayer::FormatError;
    m_player->networkStateChanged();
}
#endif

void MediaPlayerPrivateVHS::cancelLoad() {
    printf("%s\n", __PRETTY_FUNCTION__);
}

/**
 * HTMLMediaElement.play() only reaches MediaPlayerPrivate.play() if the
 * currentTime is within the buffered range.
 * It doesn't matter if the HTMLMediaElement.play() request occurs before or
 * after the seek (HTMLMediaElement.currentTime = X).
 * TODO: Is there an optimization by doing one before the other?
 */
void MediaPlayerPrivateVHS::play() {

    // TODO: signal to the MediaPlayerPlatform to start calling the buffer ready
    // callbacks, maybe through the dataloaderproxy?

    m_paused = false;
}

void MediaPlayerPrivateVHS::pause() {
    printf("%s\n", __PRETTY_FUNCTION__);
    m_paused = true;
}

// TODO: This gets called pretty frequently and it's unclear why...
// TODO: naturalSize should return the content size from the video decoder config
// need to be able to get this from the InitializationSegment(s), or possibly
// the VideoTrack impl.
FloatSize MediaPlayerPrivateVHS::naturalSize() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    return FloatSize(0, 0);
}

bool MediaPlayerPrivateVHS::hasVideo() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    if (m_dataLoaderProxy) {
        return m_dataLoaderProxy->dataSourceHasVideo();
    }
    return false;
}

bool MediaPlayerPrivateVHS::hasAudio() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    if (m_dataLoaderProxy) {
        return m_dataLoaderProxy->dataSourceHasAudio();
    }
    return false;
}

void MediaPlayerPrivateVHS::setVisible(bool visible) {
    printf("MediaPlayerPrivateVHS setVisible:%i\n", visible);
}

// TODO: m_sourceClient->durationChanged() ??
// Look up the the flow of this from the previous impl.
MediaTime MediaPlayerPrivateVHS::durationMediaTime() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    if (m_sourceClient) {
        return m_sourceClient->duration();
    }
    return MediaTime();
}

MediaTime MediaPlayerPrivateVHS::currentMediaTime() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    return MediaTime::createWithFloat(m_clockReporter->currentTime());
}

/**
 * TODO:
 * seeking is very async, AVFoundation dispatches a single shot event
 */
void MediaPlayerPrivateVHS::seekWithTolerance(const MediaTime& pos,
                                              const MediaTime& negativeTolerance,
                                              const MediaTime& positiveTolerance) {
    printf("%s pos:%lf -:%lf +:%lf\n", __PRETTY_FUNCTION__, pos.toDouble(), negativeTolerance.toDouble(), positiveTolerance.toDouble());
    printf("%s 90k PTS:%lf\n", __PRETTY_FUNCTION__, round(pos.toDouble() * 90000));
    if (!m_sourceClient) {
        printf("ERROR %s !m_sourceClient\n", __FUNCTION__);
        return;
    }
    m_clockReporter->setSeeking(true);
    /**
     * NOTE(estobbart): need to convert the time to the time base of the media
     * for example, 2.085333 * 90,000 = 187,679.97
     * then round to the nearest decimal 187,680
     * to decalre this a `start_time` in vhs, should have some rounding tolerance
     * or be able to "move" to the start time of a 'sync' sample.
     *
     * TODO: A SourceBuffer should not write util it receives buffer_ready
     * callback, which should occur after the play() call.
     * TODO: Should we even allow a sample to be enqueued prior to the
     * buffer_available callback?
     */
    // TODO: may also need to be able to report `buffered()` time range.
    // TODO: We need to keep this time and start the platform with the right value
    // it may be 0, if seek is never called and play can be called.
    m_sourceClient->seekToTime(pos);
}

bool MediaPlayerPrivateVHS::seeking() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    return m_clockReporter->seeking();
}

bool MediaPlayerPrivateVHS::paused() const {
    printf("%s %i\n", __PRETTY_FUNCTION__, m_paused);
    return m_paused;
}

MediaPlayer::NetworkState MediaPlayerPrivateVHS::networkState() const {
    printf("TODO: %s\n", __PRETTY_FUNCTION__);
    // TODO: we may not need the timer, and instead send a network state change
    // enum NetworkState { Empty, Idle, Loading, Loaded, FormatError, NetworkError, DecodeError };
    return MediaPlayer::Empty;
}

MediaPlayer::ReadyState MediaPlayerPrivateVHS::readyState() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    // enum ReadyState  { HaveNothing, HaveMetadata, HaveCurrentData, HaveFutureData, HaveEnoughData };
    return m_readyStateResponder->readyState();
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateVHS::buffered() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    if (m_sourceClient) {
        return m_sourceClient->buffered();
    }
    printf("ERROR %s !m_sourceClient\n", __FUNCTION__);
    auto timeRanges = std::make_unique<PlatformTimeRanges>();
    MediaTime rangeStart = MediaTime::zeroTime();
    MediaTime rangeEnd = MediaTime::zeroTime();
    timeRanges->add(rangeStart, rangeEnd);
    return timeRanges;
}

/**
 * Determines if the stalled event fires or if the progress event fires.
 * For MSE progress only occurs after it begins appending data, including
 * initialization data.
 *
 * What happens in VHS may not be consistent with Safari/Chrome due to requiring
 * the punch through prior to playback starting.
 *
 */
bool MediaPlayerPrivateVHS::didLoadingProgress() const {
    // TODO: Does this determine when "open" dispatches?
    printf("%s\n", __PRETTY_FUNCTION__);
    // From AVFoundation, this only happens when append is done successfully

    // TODO: it's possible the m_dataLoaderProxy gets destroyed before the
    // MediaPlayerPrivateVHS.
    // this after being constructed this gets called repeatedly, if you return true, it calls visible and setSize
    if (m_dataLoaderProxy) {
        return m_dataLoaderProxy->didLoadingProgress();
    }
    return false;
}

void MediaPlayerPrivateVHS::setSize(const IntSize& size) {
    printf("MediaPlayerPrivateVHS setSize w:%i h:%i\n",
           size.width(), size.height());
    m_size = size;
}

// This appears to do the punch through, it's unclear what the
// COORDINATED_GRAPHICS_THREADED use case is.
void MediaPlayerPrivateVHS::paint(GraphicsContext& ctx, const FloatRect& rect) {
    printf("MediaPlayerPrivateVHS paint: x:%f y:%f w:%f h:%f\n",
           rect.x(), rect.y(), rect.width(), rect.height());
    if (m_punchRect == rect) {
        return;
    }
    m_punchRect = rect;
    ctx.clearRect(m_punchRect);
}

void MediaPlayerPrivateVHS::readyStateTransition() {
    m_player->readyStateChanged();
}

}

#endif
