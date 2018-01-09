
//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "MediaPlayerPrivateHelio.h"
#include "MediaSourcePrivateHelio.h"
#include "MediaSourcePrivateClient.h"
#include <wtf/NeverDestroyed.h>
#include <stdio.h>
//#include "helio.h"
#include "UUID.h"

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
   while (true) {
       // printf("timeupdate_thread sleep 0.250\n");
       callOnMainThread([mediaPlayer]{
           static_cast<MediaPlayer*>(mediaPlayer)->timeChanged();
       });
       WTF::sleep(0.250);
   }
}


pthread_t playready_init_thread_id;

static void *playready_start(void *helioPlayer) {
    printf("MediaPlayerPrivateHelio playready_start thread START\n");
    static_cast<MediaPlayerPrivateHelio*>(helioPlayer)->decryptionSessionStarted(std::move(std::make_unique<PlayreadySession>()));
    while(static_cast<MediaPlayerPrivateHelio*>(helioPlayer)->nextDecryptionSessionTask()) {

    }
    printf("MediaPlayerPrivateHelio playready_start thread END\n");
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
              _supportsKeySystem); // MediaEngineSupportsKeySystem
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
      if (!parameters.codecs.isEmpty() && parameters.codecs != "ec-3") {
          printf("MediaPlayerPrivateHelio::MediaPlayer::IsSupported parameters.isMediaSource and codecs\n");
          return MediaPlayer::IsSupported;
      } else {
          return MediaPlayer::IsNotSupported;
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

bool MediaPlayerPrivateHelio::_supportsKeySystem(const String& keySystem, const String& mimeType) {
    printf("MediaPlayerPrivateHelio _supportsKeySystem(keySystem:%s mimeType:%s)\n", keySystem.utf8().data(), mimeType.utf8().data());
    return keySystem == "com.microsoft.playready";
}

MediaPlayerPrivateHelio::MediaPlayerPrivateHelio(MediaPlayer* player)
    : m_mediaSourceClient()
    , m_rate(0)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_mediaPlayer(player)
    , m_mediaSourcePrivate(0)
    , m_lastReportedDuration(MediaTime::zeroTime())
    // , m_prSessionLock()
    , m_emeQueue()
    , m_emeDecrypt() {
        printf("MediaPlayerPrivateHelio constructor\n");
        //m_initData = NULL;
        //m_initDataLength = 0;
#if USE(COORDINATED_GRAPHICS_THREADED)
        m_platformLayerProxy = adoptRef(new TextureMapperPlatformLayerProxy());
#endif

//#if USE(USE_HOLE_PUNCH_EXTERNAL)
#if USE(COORDINATED_GRAPHICS_THREADED)
        LockHolder locker(m_platformLayerProxy->lock());
        m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GraphicsContext3D::DONT_CARE));
#endif
//#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) && USE(PLAYREADY)
    m_prSession = nullptr;

    m_pssh = NULL;

    m_queueCond = PTHREAD_COND_INITIALIZER;
    m_queueMutex = PTHREAD_MUTEX_INITIALIZER;

    // TODO: Start PlayReady...
    pthread_create(&playready_init_thread_id, NULL, playready_start, this);

    // TOOD: this needs a thread..
    // m_emeTasks.enqueueTask([this] {
    //     LockHolder lock(&m_prSessionLock);
    //     if (m_prSession == nullptr) {
    //         m_prSession = std::make_unique<PlayreadySession>();
    //
    //         // if (!m_prSession->init()) {
    //         //     printf("PlayreadySession() failed to init");
    //         //     return MediaPlayer::InvalidPlayerState;
    //         // }
    //     }
    // });
#endif
}

MediaPlayerPrivateHelio::~MediaPlayerPrivateHelio() {
    printf("MediaPlayerPrivateHelio destructor\n");
    // TODO: m_pssh & the thread stuff
    // if (m_initData != NULL) {
    //     free(m_initData);
    // }
    // m_emeQueue.close();
    // m_emeDecrypt.close();
}

void MediaPlayerPrivateHelio::load(const String& url) {
    printf("MediaPlayerPrivateHelio load url%s\n", url.utf8().data());
}

void MediaPlayerPrivateHelio::load(const String& url, MediaSourcePrivateClient* client) {
    printf("MediaPlayerPrivateHelio load url:%s, client\n", url.utf8().data());
    m_networkState = MediaPlayer::Loading;
    m_mediaPlayer->networkStateChanged();

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
    printf("MediaPlayerPrivateHelio networkState\n");
    // enum NetworkState { Empty, Idle, Loading, Loaded, FormatError, NetworkError, DecodeError };
    return m_networkState;
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
    printf("MediaPlayerPrivateHelio didLoadingProgress\n");
    // this after being constructed this gets called repeatedly, if you return true, it calls visible and setSize
    return true;
    // This also seems to cause HTMLMediaElement to dispatch the progress event
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
    // // enum NetworkState { Empty, Idle, Loading, Loaded, FormatError, NetworkError, DecodeError };

    printf("ERROR: MediaPlayerPrivateHelio::setNetworkState not being captured!!!\n");
}

bool MediaPlayerPrivateHelio::nextDecryptionSessionTask() {
    // LockHolder lock(&m_prSessionLock);
    // if (m_prSession == nullptr || ) {
    //     return false;
    // }
    // TODO: This needs to check it's state and have a second queue for actual
    // decryption
    printf("MediaPlayerPrivateHelio::nextDecryptionSessionTask pthread_mutex_lock\n");
    pthread_mutex_lock(&m_queueMutex);
    if (m_emeQueue.empty() && (m_emeDecrypt.empty() || !m_prSession->ready())) {
        printf("MediaPlayerPrivateHelio::nextDecryptionSessionTask pthread_cond_wait\n");
        pthread_cond_wait(&m_queueCond, &m_queueMutex);
    }
    if (!m_emeQueue.empty()) {
        printf("MediaPlayerPrivateHelio::nextDecryptionSessionTask m_emeQueue task\n");
        m_emeQueue.front()(m_prSession.get());
        m_emeQueue.pop();
    }
    if (m_prSession->ready() && !m_emeDecrypt.empty()) {
        printf("MediaPlayerPrivateHelio::nextDecryptionSessionTask m_emeDecrypt task\n");
        m_emeDecrypt.front()(m_prSession.get());
        m_emeDecrypt.pop();
    }
    printf("MediaPlayerPrivateHelio::nextDecryptionSessionTask pthread_mutex_unlock\n");
    pthread_mutex_unlock(&m_queueMutex);
    return true;

    //pthread_cond_t m_queueCond;
    //pthread_mutex_t m_queueMutex;
}

void hex_dump_buffer_pssh(uint8_t *buffer, size_t size) {
    //printf("PSSH**********************\n");
    uint8_t count = 0;
    while (size) {
        printf(" %02x", buffer[0]);
        size--;
        buffer++;
        count++;
        if (count == 32) {
            printf("\n");
            count = 0;
        }
    }
    if (count) {
        printf("\n");
    }
}

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)

/**
 * Represents synchronous exceptions that can be thrown from the Encrypted
 * Media methods. This is different from the asynchronous MediaKeyError.
 *
 * enum MediaKeyException { NoError, InvalidPlayerState, KeySystemNotSupported };
 */
MediaPlayer::MediaKeyException MediaPlayerPrivateHelio::addKey(const String& keySystem,
                                                               const unsigned char* key,
                                                               unsigned keyLength,
                                                               const unsigned char* initData __attribute__((unused)),
                                                               unsigned initDataLenth __attribute__((unused)),
                                                               const String& sessionId) {
    printf("MediaPlayerPrivateHelio::addKey %s\n", keySystem.utf8().data());

    if (keySystem == "webkit-org.w3.clearkey") {
        // TODO: See JSCryptoKeySerializationJWK.h
        printf("MediaPlayerPrivateHelio::addKey webkit-org.w3.clearkey");
        hex_dump_buffer_pssh(key, keyLength);
        return MediaPlayer::NoError;
    }

    // TODO: Check that the keySystem is Playready
    // TODO: Check that the initData matches the pssh we have
    // TODO: Check that playready session is in the right state, (init'd and pending a key)
    // TODO: Check that the sessionId is the one we have for this PlayreadySession

    RefPtr<Uint8Array> keyUint8Data = Uint8Array::create(key, keyLength);

    printf("MediaPlayerPrivateHelio::addKey mutex lock\n");
    pthread_mutex_lock(&m_queueMutex);

    // TODO: weakThis
    // TODO: Am I screwing up refcounts here?
    printf("MediaPlayerPrivateHelio::addKey m_emeQueue.push\n");
    // TODO: customData??
    m_emeQueue.push([this, keySystem, keyUint8Data](PlayreadySession *prSession){
        printf("MediaPlayerPrivateHelio::addKey task\n");
        unsigned short errorCode;
        uint32_t systemCode;
        printf("MediaPlayerPrivateHelio::addKey m_prSession->playreadyProcessKey\n");
        //bool playreadyProcessKey(Uint8Array* key, unsigned short& errorCode, uint32_t& systemCode);
        bool ready = prSession->playreadyProcessKey(keyUint8Data.get(), errorCode, systemCode);
        if (ready) {
            printf("MediaPlayerPrivateHelio::addKey callOnMainThread m_mediaPlayer->keyAdded\n");
            // void keyAdded(const String& keySystem, const String& sessionId);
            callOnMainThread([this, keySystem]{
                m_mediaPlayer->keyAdded(keySystem,  m_prSession->sessionId());
            });
        } else {
            printf("MediaPlayerPrivateHelio::addKey callOnMainThread m_mediaPlayer->keyError\n");
            // void keyError(const String& keySystem, const String& sessionId, MediaPlayerClient::MediaKeyErrorCode, unsigned short systemCode);
        }
    });

    printf("MediaPlayerPrivateHelio::addKey pthread_cond_signal\n");
    pthread_cond_signal(&m_queueCond);
    printf("MediaPlayerPrivateHelio::addKey pthread_mutex_unlock\n");
    pthread_mutex_unlock(&m_queueMutex);

    return MediaPlayer::NoError;
}

MediaPlayer::MediaKeyException MediaPlayerPrivateHelio::generateKeyRequest(const String& keySystem,
                                                                           const unsigned char* initData,
                                                                           unsigned initDataLength,
                                                                           const String& customData) {
    printf("MediaPlayerPrivateHelio::generateKeyRequest %s\n", keySystem.utf8().data());
    //hex_dump_buffer_pssh(initData, initDataLength);
    if (keySystem != "com.microsoft.playready" && keySystem != "webkit-org.w3.clearkey") {
        // MediaPlayerPrivateHelio::generateKeyRequest webkit-org.w3.clearkey
        // TODO: Support ClearKey
        printf("keySystem != com.microsoft.playready && keySystem != webkit-org.w3.clearkey");
        return MediaPlayer::KeySystemNotSupported;
    }


    // NOTE(estobb200): this can vary, but generally a JS player
    // will call this method with the entire PSSH box, either From
    // a manifest, or from the result of a needKey event.
    // Worth running a few checks on this payload to see if we have
    // an entire PSSH box or not. If we do, we can use the
    // rcvmf lib to create a box.
    // TODO: Worth reading up on the spec to see if they define this
    // argument.

    if (m_pssh) {
        uint8_t *psshInitData = NULL;
        size_t size = 0;
        rcv_pssh_data(m_pssh, &psshInitData, &size);

        if (size == initDataLength && memcmp(psshInitData, initData, initDataLength) == 0) {
            printf("MediaPlayerPrivateHelio !Already generating a key for data.. ignoring generateKeyRequest\n");
            return;
        }
        printf("MediaPlayerPrivateHelio NEW PSSH!!!");
        rcv_destroy_pssh(&m_pssh);
    }

    printf("MediaPlayerPrivateHelio::generateKeyRequest init pssh\n");
    m_pssh = rcv_pssh_box_init(initData, initDataLength, NULL);

    if (!m_pssh) {
        printf("MediaPlayerPrivateHelio::generateKeyRequest ERROR !m_pssh\n");
        // TODO: Is this better as an exception or a keyError event?
        return MediaPlayer::KeySystemNotSupported;
    }

    if (keySystem == "webkit-org.w3.clearkey") {
        URL url;
        printf("MediaPlayerPrivateHelio::generateKeyRequest m_mediaPlayer->keyMessage webkit-org.w3.clearkey\n");
        m_mediaPlayer->keyMessage(keySystem, createCanonicalUUIDString(), initData, initDataLength, url);

        return MediaPlayer::NoError;
    }

    printf("MediaPlayerPrivateHelio::generateKeyRequest mutex lock\n");
    pthread_mutex_lock(&m_queueMutex);

    // TODO: weakThis
    // TODO: Am I screwing up refcounts here?
    printf("MediaPlayerPrivateHelio::generateKeyRequest m_emeQueue.push\n");
    m_emeQueue.push([this, keySystem, customData](PlayreadySession *prSession){
        printf("MediaPlayerPrivateHelio::generateKeyRequest task\n");
        uint8_t *init_data = NULL;
        size_t size = 0;
        rcv_pssh_data(m_pssh, &init_data, &size);
        RefPtr<Uint8Array> initUint8Data = Uint8Array::create(init_data, size);

        String destinationURL;
        unsigned short errorCode;
        uint32_t systemCode;
        // TODO: How does this task get the m_prSession??
        //LockHolder lock(&m_prSessionLock);
        RefPtr<Uint8Array>result = prSession->playreadyGenerateKeyRequest(initUint8Data.get(), customData, destinationURL, errorCode, systemCode);
        if (errorCode) {
            printf("ERROR playreadyGenerateKeyRequest the key request wasn't properly generated\n");
            // void MediaPlayer::keyError(const String& keySystem, const String& sessionId, MediaPlayerClient::MediaKeyErrorCode errorCode, unsigned short systemCode)
            m_mediaPlayer->keyError(keySystem, prSession->sessionId(), errorCode, systemCode);
            return;
        }

        // ?? Why was GStreamer early returning here?
        // if (prSession->ready()) {
        //     return MediaPlayer::NoError;
        // }

        URL url(URL(), destinationURL);
        printf("playready generateKeyRequest result size %u, sessionId: %s\n", result->length(), prSession->sessionId().utf8().data());

        callOnMainThread([this, keySystem, result, url]{
            m_mediaPlayer->keyMessage(keySystem, m_prSession->sessionId(), result->data(), result->length(), url);
        });
    });

    printf("MediaPlayerPrivateHelio::generateKeyRequest pthread_cond_signal\n");
    pthread_cond_signal(&m_queueCond);
    printf("MediaPlayerPrivateHelio::generateKeyRequest pthread_mutex_unlock\n");
    pthread_mutex_unlock(&m_queueMutex);

    return MediaPlayer::NoError;

}

MediaPlayer::MediaKeyException MediaPlayerPrivateHelio::cancelKeyRequest(const String& keySystem __attribute__((unused)),
                                                                         const String& sessionId __attribute__((unused))) {
    printf("ERROR: MediaPlayerPrivateHelio::cancelKeyRequest\n");
    return MediaPlayer::NoError;
}

#endif

// TODO: Move this to the PlayReadySession class
const uint8_t k_playready_system_id[] = { 0x9a, 0x04, 0xf0, 0x79,
                                           0x98, 0x40, 0x42, 0x86,
                                           0xab, 0x92, 0xe6, 0x5b,
                                           0xe0, 0x88, 0x5f, 0x95 };

// https://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1/encrypted-media/encrypted-media.html#event-summary
// TODO: Outside the ifdef so we can dispatch the right error if data
// is found in the source buffer but encryption is not supported.
// NOTE(estobb200): initData here is the whole raw pssh box with header.
void MediaPlayerPrivateHelio::sourceBufferDetectedEncryption(uint8_t *systemId, uint8_t *initData, size_t initDataLength) {
    printf("MediaPlayerPrivateHelio::sourceBufferDetectedEncryption()\n");
    // TODO: Fix playready..

    String keySystem;
    if(memcmp(systemId, k_playready_system_id, 16) == 0) {
        keySystem = String("com.microsoft.playready");
    } else {
        printf("WARN MediaPlayerPrivateHelio::sourceBufferDetectedEncryption() systemId not supported, will not dispatch keyNeeded\n");
        hex_dump_buffer_pssh(systemId, 16);
        return;
    }

    // Temporary!!!
    return;

    //hex_dump_buffer_pssh(initData, initDataLength);

    // TODO: It's possible we already have a PlayReady Session with a different
    // key. Need to ask about how to handle that.
    // TODO: Change this to the m_pssh
    // if (m_initData == NULL || m_initDataLength == 0) {
    //     m_initDataLength = initDataLength;
    //     m_initData = malloc(m_initDataLength);
    //     memcpy(m_initData, initData, m_initDataLength);
    // } else if (memcmp(m_initData, initData, m_initDataLength) == 0) {
    //     printf("MediaPlayerPrivateHelio Already genererating a key for data\n");
    //     return;
    // }

    // LockHolder lock(&m_prSessionLock);
    // if (m_prSession == nullptr) {
    //     m_prSession = std::make_unique<PlayreadySession>();
    // }

    if (!m_prSession->keyRequested()) {
        printf("MediaPlayerPrivateHelio !prSession->keyRequested()\n");
        return;
    }


    printf("MediaPlayerPrivateHelio m_mediaPlayer->keyNeeded()\n");
    //    (const String& keySystem, const String& sessionId, const unsigned char* initData, unsigned initDataLength);
    bool result = m_mediaPlayer->keyNeeded(keySystem, m_prSession->sessionId(), initData, initDataLength);

    printf("MediaPlayerPrivateHelio m_mediaPlayer->keyNeeded():%i\n", result);
}

// TODO: This should probably take a sessionId or some info based on the key?
void MediaPlayerPrivateHelio::sourceBufferRequiresDecryptionEngine(std::function<void(PlayreadySession *)> task) {
    printf("MediaPlayerPrivateHelio::sourceBufferRequiresDecryptionEngine m_emeDecrypt.push\n");
    pthread_mutex_lock(&m_queueMutex);
    m_emeDecrypt.push(task);
    printf("MediaPlayerPrivateHelio::sourceBufferRequiresDecryptionEngine pthread_cond_signal\n");
    pthread_cond_signal(&m_queueCond);
    printf("MediaPlayerPrivateHelio::sourceBufferRequiresDecryptionEngine pthread_mutex_unlock\n");
    pthread_mutex_unlock(&m_queueMutex);
}

/**
 * a SourceBufferPrivate becomes active after having reported their
 * initSegment back the SourceBuffer. We use active SourceBuffers to trigger
 * that we have metadata.
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
  // HTMLMediaElement automatically dispatches a durationChange of zero at the start
  // MediaPlayerPrivateHelio initializes with zero, and then only reports
  // valid updates.
  // TODO: Do we have to initialize with zero, or can we query for zero
  if (m_lastReportedDuration == durationMediaTime()) {
      return;
  }
  m_lastReportedDuration = durationMediaTime();
  m_mediaPlayer->durationChanged();
}

// Needed for play/pause
rcv_media_clock_controller_t * MediaPlayerPrivateHelio::platformClockController() {
    return m_platformClockController;
}

void MediaPlayerPrivateHelio::setReadyState(MediaPlayer::ReadyState stateChange) {
    // Need to print this and keep track of if anyone else is calling it..
    //if (stateChange == MediaPlayer::HaveMetadata && m_readyState < stateChange) {
    //    m_readyState = stateChange;
    //}
    printf("MediaPlayerPrivateHelio::setReadyState(MediaPlayer::ReadyState stateChange) {\n");
    m_readyState = stateChange;

    m_mediaPlayer->readyStateChanged();
}

}
