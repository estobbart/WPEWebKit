#ifndef MediaPlayerPrivateHelio_h
#define MediaPlayerPrivateHelio_h

#include "MediaPlayerPrivate.h"


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
 *
 *
 */


namespace WebCore {

class MediaSourcePrivateHelio;

class MediaPlayerPrivateHelio final : public MediaPlayerPrivateInterface {
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

    bool seeking() const override;

    bool paused() const override;

    MediaPlayer::NetworkState networkState() const override;
    MediaPlayer::ReadyState readyState() const override;

    std::unique_ptr<PlatformTimeRanges> buffered() const override;

    bool didLoadingProgress() const override;

    void setSize(const IntSize&) override;

    void paint(GraphicsContext&, const FloatRect&) override;

protected:
    void setNetworkState(MediaPlayer::NetworkState);

private:
    static void _getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types);
    static MediaPlayer::SupportsType _supportsType(const MediaEngineSupportParameters&);

    RefPtr<MediaSourcePrivateHelio> m_mediaSourcePrivate;
    // helio_t *m_helioEngine;

};

}

#endif //MediaPlayerPrivateHelio_h
