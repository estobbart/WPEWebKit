#include "config.h"

#if ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)

#include "VideoTrackPrivateHelio.h"

namespace WebCore {

VideoTrackPrivateHelio::VideoTrackPrivateHelio(void *track)
{
    //m_id = AtomicString(String::format("%i", track->id));
    m_id = AtomicString::number(0);
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)
