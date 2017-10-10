#ifndef MediaSampleHelio_H
#define MediaSampleHelio_H

#include "MediaSample.h"
//#include "demux/sample.h" // libhelio

#include <stdio.h>

#include "rcvmf_isobmff.h"

// TODO: Read up on this..

// https://developer.apple.com/documentation/coremedia/cmsamplebuffer?language=objc

namespace WebCore {

class MediaSampleHelio final : public MediaSample {

public:
    // TODO: Need the video presentation info
    static Ref<MediaSampleHelio> create(rcv_node_t *root, uint32_t timescale) {
        return adoptRef(*new MediaSampleHelio(root, timescale));
    }

private:
    MediaSampleHelio(rcv_node_t *root, uint32_t timescale)
      : m_presentationTime()
      , m_decodeTime()
      , m_duration() {
        m_sample = root;
        //m_id = AtomicString(String::format("%i", m_sample->track_id));
        // m_id = AtomicString::number(0);

        rcv_node_t *box = rcv_node_child(m_sample, "tfhd");
        if (box) {
            rcv_tfhd_box_t *tfhd = RCV_TFHD_BOX(rcv_node_raw(box));
            m_id = AtomicString::number(rcv_tfhd_track_id(tfhd));
        } else {
            printf("ERROR UNABLE TO GET rcv_tkhd_track_id\n");
            m_id = AtomicString::number(0);
        }

        m_timescale = timescale;
        //printf("MediaSampleHelio m_timescale %u timescale %u\n", m_timescale, timescale);
    }

    virtual ~MediaSampleHelio() {
        printf("~MediaSampleHelio");
        rcv_destory_tree(&m_sample);
    }

    MediaTime presentationTime() const override;
    MediaTime decodeTime() const override;
    MediaTime duration() const override;

    AtomicString trackID() const override {
        return m_id;
    }

    // TODO: TFHD is it's track ID, we shouldn't support set here.
    // TODO: Why is this part of the interface?
    void setTrackID(const String& id) override {
        printf("WARNING MediaSampleHelio::setTrackID %s NOT SUPPORTED", id.utf8().data());
        printf("... current id %s \n", m_id.string().utf8().data());
    }

    size_t sizeInBytes() const override;
    // it's width, height if video
    // TODO: Need to get this from the init header..
    FloatSize presentationSize() const override;
    void offsetTimestampsBy(const MediaTime&) override;
    // TODO: When is setTimestamps called??
    void setTimestamps(const MediaTime&, const MediaTime&) override;
    bool isDivisable() const override;
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime) override;
    // TODO: What's a non-displaying copy for?
    Ref<MediaSample> createNonDisplayingCopy() const override;

    SampleFlags flags() const override;
    // TODO: Define PlatformSample for Helio
    PlatformSample platformSample() override;

    // TODO: Support debug of the sample
    void dump(PrintStream&) const override;

    // RetainPtr<CMSampleBufferRef> m_sample;
    AtomicString m_id;

    rcv_node_t *m_sample;

    uint32_t m_timescale;
  
    MediaTime m_presentationTime;
    MediaTime m_decodeTime;
    MediaTime m_duration;
};

}

#endif
