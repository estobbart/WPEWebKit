#ifndef AudioTrackPrivateVHS_h
#define AudioTrackPrivateVHS_h

#if ENABLE(VIDEO_TRACK)

#include "AudioTrackPrivate.h"

namespace WebCore {

class AudioTrackPrivateVHS final : public AudioTrackPrivate {
public:
    static RefPtr<AudioTrackPrivateVHS> create(uint32_t id) {
        return adoptRef(new AudioTrackPrivateVHS(id));
    }

    AtomicString id() const override { return m_id; }

private:
    AudioTrackPrivateVHS(uint32_t id);

    AtomicString m_id;

};

}

#endif // ENABLE(VIDEO_TRACK)

#endif // AudioTrackPrivateVHS_h
