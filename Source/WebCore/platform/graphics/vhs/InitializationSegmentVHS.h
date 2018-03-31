#ifndef InitializationSegmentVHS_h
#define InitializationSegmentVHS_h

#include "MediaSampleVHS.h"
#include "MediaSegmentVHS.h"
#include <wtf/RefCounted.h>

// TODO: move these to one vhs_formats include file
#include <vhs/aac.h>
#include <vhs/avc.h>

namespace WebCore {

class InitializationSegmentVHS final : public MediaSampleVHSDefaultsProxy {

public:
    static RefPtr<InitializationSegmentVHS> create(Ref<MediaSegmentVHS>&& segment) {
        return adoptRef(new InitializationSegmentVHS(WTFMove(segment)));
    }

    // Only mvhd timescale is returned here
    // see mediaTimescale for potential track overrides
    uint32_t timescale() const;

    // mvhd, or mehd
    // if the duration can not be determined (live) ULLONG_MAX is returned
    uint64_t duration() const;

    uint32_t trackId() const;

    // const char * codec() const;

    // The Initialization Segment for ISOBMFF contains defaults for the
    // Media Segments. Not to be confused with the above timescale & duration.
    uint32_t mediaTimescale() const override;
    uint64_t sampleDuration() const override;
    FloatSize trackPresentationSize() const override;
    VHSSampleType sampleType() const override;
    void * decoderConfig() const override;

private:
    InitializationSegmentVHS(Ref<MediaSegmentVHS>&& segment);

    virtual ~InitializationSegmentVHS() { }

    Ref<MediaSegmentVHS> m_segment;

    vhs_aac_config_t *m_audioConfig;
    vhs_param_sets_t *m_videoConfig;

    // cached values
    mutable uint32_t m_timescale;
    mutable uint64_t m_duration;
    mutable uint32_t m_trackId;
};

} // webcore

#endif
