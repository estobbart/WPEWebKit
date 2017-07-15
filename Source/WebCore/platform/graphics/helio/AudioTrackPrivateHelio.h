#ifndef AudioTrackPrivateHelio_h
#define AudioTrackPrivateHelio_h

#include "AudioTrackPrivate.h"
#include "demux/track.h"

#if ENABLE(VIDEO_TRACK)

namespace WebCore {
class AudioTrackPrivateHelio final : public AudioTrackPrivate {
public:
    static PassRefPtr<AudioTrackPrivateHelio> create(helio_track_t *track) {
        return adoptRef(new AudioTrackPrivateHelio(track));
    }
    // The id of the AudioTrackPrivate is used by the SourceBuffer to
    // create a track buffer map that it can associate MediaSamples with.
    AtomicString id() const override { return m_id; }

private:
    AudioTrackPrivateHelio(helio_track_t *track);

    AtomicString m_id;
};


}

#endif // ENABLE(VIDEO_TRACK)

#endif // AudioTrackPrivateHelio_h
