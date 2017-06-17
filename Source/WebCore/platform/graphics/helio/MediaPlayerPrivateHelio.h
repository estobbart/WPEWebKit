#ifndef MediaPlayerPrivateHelio_h
#define MediaPlayerPrivateHelio_h

#include "MediaPlayerPrivate.h"

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
