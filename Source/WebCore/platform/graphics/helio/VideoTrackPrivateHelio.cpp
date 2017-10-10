#include "config.h"

#if ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)

#include "VideoTrackPrivateHelio.h"

namespace WebCore {

VideoTrackPrivateHelio::VideoTrackPrivateHelio(uint32_t id) {
    m_id = AtomicString::number(id);
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)
