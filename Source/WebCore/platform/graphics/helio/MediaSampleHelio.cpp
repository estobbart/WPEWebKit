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
    return MediaTime::createWithDouble(m_sample->pts * m_sample->scale);
    //return toMediaTime(CMSampleBufferGetPresentationTimeStamp(m_sample.get()));
}

MediaTime MediaSampleHelio::decodeTime() const {
    // printf("decodeTime\n");
    return MediaTime::createWithDouble(m_sample->pts * m_sample->scale);
}

MediaTime MediaSampleHelio::duration() const {
    // printf("duration\n");
    return MediaTime::createWithDouble(m_sample->duration * m_sample->scale);
}

size_t MediaSampleHelio::sizeInBytes() const {
    printf("sizeInBytes\n");
    return m_sample->size;
}

FloatSize MediaSampleHelio::presentationSize() const {
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
    return true;
}

std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> MediaSampleHelio::divide(const MediaTime& presentationTime __attribute__((unused))) {
    printf("divide\n");
    return { nullptr, nullptr };
}

Ref<MediaSample> MediaSampleHelio::createNonDisplayingCopy() const {
    printf("createNonDisplayingCopy\n");
    return MediaSampleHelio::create(m_sample);
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
