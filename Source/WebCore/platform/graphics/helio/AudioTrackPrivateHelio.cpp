#include "config.h"

#if ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)

#include "AudioTrackPrivateHelio.h"

namespace WebCore {

AudioTrackPrivateHelio::AudioTrackPrivateHelio(helio_track_t *track)
{
    m_id = AtomicString::number(track->id);
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)
