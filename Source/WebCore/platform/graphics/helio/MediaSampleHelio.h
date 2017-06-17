#ifndef MediaSampleHelio_H
#define MediaSampleHelio_H

#include "MediaSample.h"

// TODO: include helio
// TODO: Read up on this..

// https://developer.apple.com/documentation/coremedia/cmsamplebuffer?language=objc

namespace WebCore {

class MediaSampleHelio final : public MediaSample {

public:
    static Ref<MediaSampleHelio> create() { return adoptRef(*new MediaSampleHelio()); }

private:
    MediaSampleHelio() {}

    virtual ~MediaSampleHelio() { }

    MediaTime presentationTime() const override;
    MediaTime decodeTime() const override;
    MediaTime duration() const override;

    AtomicString trackID() const override { return m_id; }
    void setTrackID(const String& id) override { m_id = id; }

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
};

}

#endif
