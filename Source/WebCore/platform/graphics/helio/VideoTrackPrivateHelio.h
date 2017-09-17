#ifndef VideoTrackPrivateHelio_h
#define VideoTrackPrivateHelio_h

#include "VideoTrackPrivate.h"
//#include "demux/track.h"

#if ENABLE(VIDEO_TRACK)

namespace WebCore {

class VideoTrackPrivateHelio final : public VideoTrackPrivate {
public:
    static PassRefPtr<VideoTrackPrivateHelio> create(void *track) {
        return adoptRef(new VideoTrackPrivateHelio(track));
    }

    AtomicString id() const override { return m_id; }

private:
    VideoTrackPrivateHelio(void *track);

    AtomicString m_id;

};

}

#endif // ENABLE(VIDEO_TRACK)

#endif // VideoTrackPrivateHelio_h
