#include "config.h"

#if ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)

#include "AudioTrackPrivateHelio.h"

namespace WebCore {

AudioTrackPrivateHelio::AudioTrackPrivateHelio(uint32_t id)
{
    //m_id = AtomicString::number(track->id);
    m_id = AtomicString::number(id);
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && ENABLE(VIDEO_TRACK)
