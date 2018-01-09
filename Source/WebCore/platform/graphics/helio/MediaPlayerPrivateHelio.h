#ifndef MediaPlayerPrivateHelio_h
#define MediaPlayerPrivateHelio_h

#include "MediaPlayerPrivate.h"
#include "rcvmf_media_pipeline.h"

#include "PlatformLayer.h"

//#if USE(TEXTURE_MAPPER)
//#include "TextureMapperPlatformLayer.h"
#include "TextureMapperPlatformLayerProxy.h"
//#endif

#include "PlayreadySession.h"

#include <pthread.h>
#include <queue>
#include "rcvmf_isobmff.h"

/**
 * The MediaPlayerPrivateHelio gets registered in platform/graphics/MediaPlayer.cpp
 * When JavaScript creates a `var ms = new MediaSource()` then creates a URL
 * with that source `URL.createObjectURL(ms)`. When this MediaSource gets
 * assigned to the video.src attribute, MediaPlayerPrivateHelio::load
 * is called with the url of the domain creating the URL.
 * MediaPlayerPrivateHelio is chosen by first calling it's _supportsType function.
 * During the load function the MediaPlayerPrivateHelio creates
 * a MediaSourcePrivateHelio, passing itself & the MediaSourcePrivateClient
 * it receives in the load function. MediaSourcePrivateHelio then uses the
 * client to call setPrivateAndOpen with a reference to itself. This dispatches
 * the `sourceopen` event at the MediaSource object created earlier.
 *
 * Once the sourceopen event fires, a JavaScript player typically creates
 * a SourceBuffer by calling `addSourceBuffer` of the `MediaSource`, which
 * in this case uses our MediaSourcePrivateHelio that was set as the private
 * of the MediaSource earlier. MediaSourcePrivateHelio's addSourceBuffer
 * creates a SourceBufferPrivateHelio object which gets returned to the
 * MediaSource. MediaSource then creates a SourceBuffer object, passing it
 * a reference to the SourceBufferPrivateHelio, SourceBuffer then calls it's
 * private reference setting itself as the client.
 *
 * Once JavaScript has a SourceBuffer object, is can begin to append data.
 * The append call proxy's through the SourceBuffer to the
 * SourceBufferPrivateHelio, once the data has been demuxed, the private
 * notify's the client that it first found an initialization segment,
 * which includes metadata about the demuxed data. Then beings providing samples
 * for that metadata. When it's done with the provided data it calls the clients
 * sourceBufferPrivateAppendComplete which causes the updatend event to fire.
 *
 * MediaPlayerHelio has a reference to the clock control component of the
 * video engine.
 *
 */


namespace WebCore {

class MediaSourcePrivateHelio;

class MediaPlayerPrivateHelio final : public MediaPlayerPrivateInterface
#if USE(COORDINATED_GRAPHICS_THREADED)
    , public PlatformLayer
#endif
{
public:
    explicit MediaPlayerPrivateHelio(MediaPlayer*);
    virtual ~MediaPlayerPrivateHelio();

    // This gets called by MediaPlayer, MediaPlayer uses this to determine
    // the best engine when calling isTypeSupported
    static void registerMediaEngine(MediaEngineRegistrar);

    void load(const String& url) override;
#if ENABLE(MEDIA_SOURCE)
    void load(const String& url, MediaSourcePrivateClient*) override;
#endif
#if ENABLE(MEDIA_STREAM)
    void load(MediaStreamPrivate&) override { setNetworkState(MediaPlayer::FormatError); }
#endif
    void cancelLoad() override;

    void play() override;
    void pause() override;

    FloatSize naturalSize() const override;

    bool hasVideo() const override;
    bool hasAudio() const override;

    void setVisible(bool) override;

    /**
     * Duration is stored in the MediaSourcePrivateClient
     */
    MediaTime durationMediaTime() const;

    // TODO: We may want to support these as doubles
    virtual float currentTime() const;

    /**
     * Not required to override, but seeking is nice..
     */
    //virtual void seek(float) { }
    void seekDouble(double time); // { seek(time); }
    void seek(const MediaTime& time) { seekDouble(time.toDouble()); }
    void seekWithTolerance(const MediaTime& time, const MediaTime&, const MediaTime&) { seek(time); }

    bool seeking() const override;

    bool paused() const override;

    MediaPlayer::NetworkState networkState() const override;
    MediaPlayer::ReadyState readyState() const override;

    // Looks like this only gets used when calculating memory cost in MediaPlayerPrivate.h
    std::unique_ptr<PlatformTimeRanges> buffered() const override { return m_mediaPlayer->buffered(); }

    bool didLoadingProgress() const override;

    void setSize(const IntSize&) override;

    void paint(GraphicsContext&, const FloatRect&) override;

#if USE(COORDINATED_GRAPHICS_THREADED)
    PlatformLayer* platformLayer() const override { return const_cast<MediaPlayerPrivateHelio*>(this); }
    bool supportsAcceleratedRendering() const override { return false; }
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
    /**
     // Represents synchronous exceptions that can be thrown from the Encrypted Media methods.
     // This is different from the asynchronous MediaKeyError.
     enum MediaKeyException { NoError, InvalidPlayerState, KeySystemNotSupported };
     */
    MediaPlayer::MediaKeyException addKey(const String&, const unsigned char*, unsigned, const unsigned char*, unsigned, const String&) override;
    //{ return MediaPlayer::KeySystemNotSupported; }
    MediaPlayer::MediaKeyException generateKeyRequest(const String&, const unsigned char*, unsigned, const String&) override;
    //{ return MediaPlayer::KeySystemNotSupported; }
    MediaPlayer::MediaKeyException cancelKeyRequest(const String&, const String&) override;
    //{ return MediaPlayer::KeySystemNotSupported; }
#endif

    // TODO: This object as a proxy, is a little ugly.. may make more sense to
    // have the SourceBuffer call MediaPlayerPrivate methods directly.

    // To be able to report buffer status back to the MediaPlayer->client()
    // this method is exposed to the MediaSource can call it.
    void mediaSourcePrivateActiveSourceBuffersChanged();

    // This proxies from the MediaSourcePrivate which receives the first
    // durationChanged call, through this object to the MediaPlayer
    void durationChanged();

    rcv_media_clock_controller_t * platformClockController();

    void sourceBufferDetectedEncryption(uint8_t *systemId, uint8_t *initData, size_t initDataLength);

    void sourceBufferRequiresDecryptionEngine(std::function<void(PlayreadySession *)> task);

    // Called by the SourceBuffer's to decrypt content..
    // PlayreadySession* decryptionSession() { return prSession; }

    void decryptionSessionStarted(std::unique_ptr<PlayreadySession> playReady) {
        printf("MediaPlayerPrivateHelio::decryptionSessionStarted\n");
        //LockHolder lock(&m_prSessionLock);
        if (m_prSession == nullptr) {
            printf("MediaPlayerPrivateHelio::decryptionSessionStarted m_prSession assignment\n");
            m_prSession = std::move(playReady);
        }
    }

    bool nextDecryptionSessionTask();

protected:
    void setNetworkState(MediaPlayer::NetworkState);

private:

    void setReadyState(MediaPlayer::ReadyState);

    MediaSourcePrivateClient* m_mediaSourceClient;

    MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;

    uint8_t m_rate;

    static void _getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types);
    static MediaPlayer::SupportsType _supportsType(const MediaEngineSupportParameters&);
    static bool _supportsKeySystem(const String& keySystem, const String& mimeType); // MediaEngineSupportsKeySystem

    RefPtr<MediaSourcePrivateHelio> m_mediaSourcePrivate;
    // helio_t *m_helioEngine;

    MediaPlayer *m_mediaPlayer;
    rcv_media_clock_controller_t *m_platformClockController;

    // See MSE_Helio.md for notes about duration & durationChange events.
    MediaTime m_lastReportedDuration;

    IntSize m_size;
#if USE(COORDINATED_GRAPHICS_THREADED)
    RefPtr<TextureMapperPlatformLayerProxy> proxy() const override { return m_platformLayerProxy.copyRef(); }
    void swapBuffersIfNeeded() override { };
    void pushTextureToCompositor();
    RefPtr<TextureMapperPlatformLayerProxy> m_platformLayerProxy;
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) && USE(PLAYREADY)
    std::unique_ptr<PlayreadySession> m_prSession;
    // This lock is only used to access the m_prSession
    //mutable Lock m_prSessionLock;

    // TODO: Move this into the PlayReady lib??
    // uint8_t *m_initData;
    // size_t m_initDataLength;

    rcv_pssh_box_t *m_pssh;

    std::queue<std::function<void(PlayreadySession *)>> m_emeQueue;
    std::queue<std::function<void(PlayreadySession *)>> m_emeDecrypt;
    pthread_cond_t m_queueCond;
    pthread_mutex_t m_queueMutex;


#endif

};

}

#endif //MediaPlayerPrivateHelio_h
