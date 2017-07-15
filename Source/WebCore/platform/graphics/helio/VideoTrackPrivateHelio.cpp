#include "config.h"

#if ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)

#include "VideoTrackPrivateHelio.h"

namespace WebCore {

VideoTrackPrivateHelio::VideoTrackPrivateHelio(helio_track_t *track)
{
    //m_id = AtomicString(String::format("%i", track->id));
    m_id = AtomicString::number(track->id);
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)
