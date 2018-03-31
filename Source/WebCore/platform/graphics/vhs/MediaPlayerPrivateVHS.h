/*
*/

#ifndef MediaPlayerPrivateVHS_h
#define MediaPlayerPrivateVHS_h

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include "MediaPlayerPrivate.h"
#include "ReadyStateResponder.h"

// TODO: Clean up the macro usage here..
// #include "PlatformLayer.h"
//
// #if USE(COORDINATED_GRAPHICS_THREADED)
// #include "TextureMapperPlatformLayerProxy.h"
// #endif


/**
 * The MediaPlayerPrivateVHS gets registered in platform/graphics/MediaPlayer.cpp
 * When JavaScript creates a `var ms = new MediaSource()` then creates a URL
 * with that source `URL.createObjectURL(ms)`. When this MediaSource gets
 * assigned to the video.src attribute, MediaPlayerPrivateHelio::load
 * is called with the url of the domain creating the URL.
 *
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
 *
 *
 */


namespace WebCore {

class MediaSourcePrivateVHS;
class MediaClockReporter;
class DataLoaderProxy;

class MediaPlayerPrivateVHS final : public MediaPlayerPrivateInterface, public ReadyStateClient
// #if USE(COORDINATED_GRAPHICS_THREADED)
//     , public PlatformLayer
// #endif
{
public:
    explicit MediaPlayerPrivateVHS(MediaPlayer*);
    virtual ~MediaPlayerPrivateVHS();

    // This gets called by MediaPlayer, MediaPlayer uses this to determine
    // the best engine when calling isTypeSupported
    static void registerMediaEngine(MediaEngineRegistrar);

    /**
     * Not supported, network state will be set to format error.
     */
    void load(const String& url) override;
    /**
     * Called when a blob assignment occurs on an HTMLMediaElement,
     * after checking the registered engine supportsType function.
     */
    void load(const String& url, MediaSourcePrivateClient*) override;

#if ENABLE(MEDIA_STREAM)
    /**
     * Not supported, network state will be set to format error.
     */
    void load(MediaStreamPrivate&) override;
#endif

    void cancelLoad() override;

    void play() override;
    void pause() override;

    //
    // NOTE(estobbart): naturalSize is expected to return the video track size
    // in the decoder config, regardless of what the setSize argument is.
    //
    FloatSize naturalSize() const override;

    bool hasVideo() const override;
    bool hasAudio() const override;

    void setVisible(bool) override;

    // declared optional in the interface, default return 0
    // TODO: Look up how duration was being returned in the STB impl.
    MediaTime durationMediaTime() const override;

    // declared optional in the interface, default return 0
    MediaTime currentMediaTime() const override;

    // declared optional in the interface, default no-op
    void seekWithTolerance(const MediaTime&, const MediaTime&, const MediaTime&) override;

    bool seeking() const override;

    // TODO: support rate changes..
    // virtual void setRate(float) { }
    // virtual void setRateDouble(double rate) { setRate(rate); }
    // virtual double rate() const { return 0; }

    bool paused() const override;

    MediaPlayer::NetworkState networkState() const override;
    MediaPlayer::ReadyState readyState() const override;

    std::unique_ptr<PlatformTimeRanges> buffered() const override;

    bool didLoadingProgress() const override;

    void setSize(const IntSize&) override;

    void paint(GraphicsContext&, const FloatRect&) override;

// #if USE(COORDINATED_GRAPHICS_THREADED)
//     virtual PlatformLayer* platformLayer() const override { return const_cast<MediaPlayerPrivateVHS*>(this); }
//     virtual bool supportsAcceleratedRendering() const override { return true; }
// #endif

    // ReadyStateClient overrides
    void readyStateTransition() override;

private:
    /**
     * MediaPlayer engine registration functions
     */
    static void _getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types);
    static MediaPlayer::SupportsType _supportsType(const MediaEngineSupportParameters&);
    static bool _supportsKeySystem(const String& keySystem, const String& mimeType);

    // Provided in the constructor when created from the engine registration
    MediaPlayer* m_player;
    bool m_paused;
    IntSize m_size;
    FloatRect m_punchRect;
    RefPtr<MediaClockReporter> m_clockReporter;

    // TODO: Do I need to keep a reference here?
    //RefPtr<MediaSourcePrivateVHS> m_mediaSourcePrivate;
    RefPtr<MediaSourcePrivateClient> m_sourceClient;

    MediaPlayer::NetworkState m_networkState;

    RefPtr<DataLoaderProxy> m_dataLoaderProxy;
    RefPtr<ReadyStateResponder> m_readyStateResponder;

// #if USE(COORDINATED_GRAPHICS_THREADED)
//     RefPtr<TextureMapperPlatformLayerProxy> proxy() const override { return m_platformLayerProxy.copyRef(); }
//     void swapBuffersIfNeeded() override { };
//     void pushTextureToCompositor();
//
//     RefPtr<TextureMapperPlatformLayerProxy> m_platformLayerProxy;
// #endif

};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)

#endif //MediaPlayerPrivateVHS_h
