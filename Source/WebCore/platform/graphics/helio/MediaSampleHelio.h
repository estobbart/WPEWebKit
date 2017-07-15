#ifndef MediaSampleHelio_H
#define MediaSampleHelio_H

#include "MediaSample.h"
#include "demux/sample.h" // libhelio

#include <stdio.h>

// TODO: Read up on this..

// https://developer.apple.com/documentation/coremedia/cmsamplebuffer?language=objc

namespace WebCore {

class MediaSampleHelio final : public MediaSample {

public:
    static Ref<MediaSampleHelio> create(helio_sample_t *sample) {
        return adoptRef(*new MediaSampleHelio(sample));
    }

private:
    MediaSampleHelio(helio_sample_t *sample) {
        m_sample = sample;
        //m_id = AtomicString(String::format("%i", m_sample->track_id));
        m_id = AtomicString::number(m_sample->track_id);
    }

    virtual ~MediaSampleHelio() { }

    MediaTime presentationTime() const override;
    MediaTime decodeTime() const override;
    MediaTime duration() const override;

    AtomicString trackID() const override {
        //printf("MediaSampleHelio::trackID %s \n", m_id.string().utf8().data());
        return m_id;
    }
    void setTrackID(const String& id) override {
        m_id = id;
    }

    size_t sizeInBytes() const override;
    // it's width, height if video
    FloatSize presentationSize() const override;
    void offsetTimestampsBy(const MediaTime&) override;
    void setTimestamps(const MediaTime&, const MediaTime&) override;
    bool isDivisable() const override;
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime) override;
    Ref<MediaSample> createNonDisplayingCopy() const override;


    SampleFlags flags() const override;
    PlatformSample platformSample() override;

    void dump(PrintStream&) const override;

    // RetainPtr<CMSampleBufferRef> m_sample;
    AtomicString m_id;

    helio_sample_t *m_sample;
};

}

#endif
