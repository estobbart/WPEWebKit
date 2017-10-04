#import "config.h"
#import "MediaSampleHelio.h"

#import <wtf/PrintStream.h>

/**
 * presentationTime
 * decodeTime
 * duration
 */

namespace WebCore {

MediaTime MediaSampleHelio::presentationTime() const {
    // printf("presentationTime\n");
    rcv_node_t *box = rcv_node_child(m_sample, "trun");
    if (box) {
        rcv_trun_box_t *trun = RCV_TRUN_BOX(rcv_node_raw(box));
        MediaTime _decodeTime = this->decodeTime();
        double pts = rcv_trun_first_presentation_time(trun, _decodeTime.toDouble(), m_timescale);
        //printf("m_timescale %u\n", m_timescale);
        printf("PRESENTATION TIME:%f\n", pts);
        return MediaTime::createWithDouble(pts);
    }

    // double rcv_trun_first_presentation_time(rcv_trun_box_t *trun, double decode_time, uint32_t timescale);
    return MediaTime::createWithDouble(0);
    //return toMediaTime(CMSampleBufferGetPresentationTimeStamp(m_sample.get()));
}

MediaTime MediaSampleHelio::decodeTime() const {
    rcv_node_t *box = rcv_node_child(m_sample, "tfdt");
    if (box) {
        rcv_tfdt_box_t *tfdt = RCV_TFDT_BOX(rcv_node_raw(box));
        double dts = rcv_tfdt_decode_time_seconds(tfdt, m_timescale);
        //printf("m_timescale %u\n", m_timescale);
        printf("DECODE TIME:%f\n", dts);
        return MediaTime::createWithDouble(dts);
    }
    // printf("decodeTime\n");
    return MediaTime::createWithDouble(0);
}

MediaTime MediaSampleHelio::duration() const {
    // printf("duration\n");
    rcv_node_t *box = rcv_node_child(m_sample, "trun");
    if (box) {
        rcv_trun_box_t *trun = RCV_TRUN_BOX(rcv_node_raw(box));
        double duration = rcv_trun_duration(trun, m_timescale);
        //printf("m_timescale %u\n", m_timescale);
        printf("DURATION TIME:%f\n", duration);
        return MediaTime::createWithDouble(duration);
    }
    return MediaTime::createWithDouble(0);
}

size_t MediaSampleHelio::sizeInBytes() const {
    rcv_node_t *node = rcv_node_child(m_sample, "mdat");
    if (node) {
        rcv_mdat_box_t *mdat = RCV_MDAT_BOX(rcv_node_raw(node));
        size_t size = rcv_mdat_data_size(mdat);
        printf("sizeInBytes:%zu \n", size);
        return size;
    }
    return 0;
}

FloatSize MediaSampleHelio::presentationSize() const {
    // TODO: This is only in the init header
    printf("presentationSize\n");
    return FloatSize();
}

void MediaSampleHelio::offsetTimestampsBy(const MediaTime & mediaTime __attribute__((unused))) {
  printf("offsetTimestampsBy\n");
}

void MediaSampleHelio::setTimestamps(const MediaTime &mediaTimeOne __attribute__((unused)),
                                     const MediaTime &mediaTimeTwo __attribute__((unused))) {
  printf("setTimestamps\n");
}

bool MediaSampleHelio::isDivisable() const {
    printf("isDivisable\n");
    return false;
}

std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> MediaSampleHelio::divide(const MediaTime& presentationTime __attribute__((unused))) {
    printf("divide\n");
    return { nullptr, nullptr };
}

Ref<MediaSample> MediaSampleHelio::createNonDisplayingCopy() const {
    printf("createNonDisplayingCopy\n");
    return MediaSampleHelio::create(NULL, 0);
}

MediaSample::SampleFlags MediaSampleHelio::flags() const {
    printf("flags\n");
    // TODO: It's unclear what this IsSync means
    /*
    enum SampleFlags {
        None = 0,
        IsSync = 1 << 0,
        IsNonDisplaying = 1 << 1,
    };
    */
    return MediaSample::IsSync;
}

PlatformSample MediaSampleHelio::platformSample() {
    printf("platformSample\n");
    return PlatformSample();
}

void MediaSampleHelio::dump(PrintStream &ps __attribute__((unused))) const {
    // out.print("{PTS(", presentationTime(), "), DTS(", decodeTime(), "), duration(", duration(), "), flags(", (int)flags(), "), presentationSize(", presentationSize().width(), "x", presentationSize().height(), ")}");
    printf("dump\n");
}

}
