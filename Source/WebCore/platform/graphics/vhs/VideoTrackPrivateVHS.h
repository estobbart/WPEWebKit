#ifndef VideoTrackPrivateVHS_h
#define VideoTrackPrivateVHS_h

#if ENABLE(VIDEO_TRACK)

#include "VideoTrackPrivate.h"

namespace WebCore {

class VideoTrackPrivateVHS final : public VideoTrackPrivate {
public:
    static RefPtr<VideoTrackPrivateVHS> create(uint32_t id) {
        return adoptRef(new VideoTrackPrivateVHS(id));
    }

    AtomicString id() const override { return m_id; }

private:
    VideoTrackPrivateVHS(uint32_t id);

    AtomicString m_id;

};

}

#endif // ENABLE(VIDEO_TRACK)

#endif // VideoTrackPrivateVHS_h
