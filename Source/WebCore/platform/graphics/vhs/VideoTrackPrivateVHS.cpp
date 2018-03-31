#include "config.h"
#include "VideoTrackPrivateVHS.h"

#if ENABLE(VIDEO_TRACK)

namespace WebCore {

VideoTrackPrivateVHS::VideoTrackPrivateVHS(uint32_t id) {
    m_id = AtomicString::number(id);
}

} // namespace WebCore

#endif // ENABLE(VIDEO_TRACK)
