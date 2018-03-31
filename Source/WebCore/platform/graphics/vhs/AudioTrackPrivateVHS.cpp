#include "config.h"
#include "AudioTrackPrivateVHS.h"

#if ENABLE(VIDEO_TRACK)

namespace WebCore {

AudioTrackPrivateVHS::AudioTrackPrivateVHS(uint32_t id) {
    m_id = AtomicString::number(id);
}

} // namespace WebCore

#endif // ENABLE(VIDEO_TRACK)
