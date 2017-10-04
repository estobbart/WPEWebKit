#ifndef VideoTrackPrivateHelio_h
#define VideoTrackPrivateHelio_h

#include "VideoTrackPrivate.h"
//#include "demux/track.h"

#if ENABLE(VIDEO_TRACK)

namespace WebCore {

class VideoTrackPrivateHelio final : public VideoTrackPrivate {
public:
    static PassRefPtr<VideoTrackPrivateHelio> create(uint32_t id) {
        return adoptRef(new VideoTrackPrivateHelio(id));
    }

    AtomicString id() const override { return m_id; }

private:
    VideoTrackPrivateHelio(uint32_t id);

    AtomicString m_id;

};

}

#endif // ENABLE(VIDEO_TRACK)

#endif // VideoTrackPrivateHelio_h
