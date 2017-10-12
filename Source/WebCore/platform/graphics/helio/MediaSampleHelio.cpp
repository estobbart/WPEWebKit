#import "config.h"
#import "MediaSampleHelio.h"

#import <wtf/PrintStream.h>

namespace WebCore {

MediaTime MediaSampleHelio::presentationTime() const {
    if (!m_presentationTime) {
        rcv_node_t *box = rcv_node_child(m_sample, "trun");
        if (box) {
            rcv_trun_box_t *trun = RCV_TRUN_BOX(rcv_node_raw(box));
            MediaTime _decodeTime = this->decodeTime();
            double pts = rcv_trun_first_presentation_time(trun, _decodeTime.toDouble(), m_timescale);
            printf("PRESENTATION TIME:%f\n", pts);
            m_presentationTime = MediaTime::createWithDouble(pts);
        }
    }
    return m_presentationTime;
}

MediaTime MediaSampleHelio::decodeTime() const {
    if (!m_decodeTime) {
        rcv_node_t *box = rcv_node_child(m_sample, "tfdt");
        if (box) {
            rcv_tfdt_box_t *tfdt = RCV_TFDT_BOX(rcv_node_raw(box));
            double dts = rcv_tfdt_decode_time_seconds(tfdt, m_timescale);
            printf("DECODE TIME:%f\n", dts);
            m_decodeTime = MediaTime::createWithDouble(dts);
        }
    }
    return m_decodeTime;
}

MediaTime MediaSampleHelio::duration() const {
    if (!m_duration) {
        rcv_node_t *box = rcv_node_child(m_sample, "trun");
        if (box) {
            rcv_trun_box_t *trun = RCV_TRUN_BOX(rcv_node_raw(box));
            double duration = rcv_trun_duration(trun, m_timescale);
            printf("DURATION TIME:%f\n", duration);
            m_duration = MediaTime::createWithDouble(duration);
        }
    }
    return m_duration;
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
    // TODO: This is only in the init header, would need the SourceBuffer
    // to provide it in the constructor.
    printf("presentationSize\n");
    return FloatSize();
}

void MediaSampleHelio::offsetTimestampsBy(const MediaTime & mediaTime __attribute__((unused))) {
  // TODO: We need to support this at some point.
  printf("offsetTimestampsBy\n");
}

void MediaSampleHelio::setTimestamps(const MediaTime &mediaTimeOne __attribute__((unused)),
                                     const MediaTime &mediaTimeTwo __attribute__((unused))) {
  printf("setTimestamps\n");
}

// NOTE(estobb200): A sample would only be divisible if it had multiple
// keyframes in it that could be used as RAPs. Since in S8 the keyframes are
// only at the start of a sample it's not worth dividing them. We do need to
// support an iframe track, and in that case would get multiple keyframes in
// a sample. But it still TDB if it's worth dividing them.
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

// NOTE(estobb200): Every sample from super8 is considered a sync sample,
// See comments about being divisible.
MediaSample::SampleFlags MediaSampleHelio::flags() const {
    printf("flags\n");
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
