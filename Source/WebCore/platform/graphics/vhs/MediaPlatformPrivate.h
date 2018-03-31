#ifndef MediaPlatformPrivate_h
#define MediaPlatformPrivate_h

#include <vhs/vhs_media_pipeline.h>

namespace WebCore {

// TODO: Move this..
class VHSMediaPipeline {
public:
    virtual ~VHSMediaPipeline() {}

    virtual vhs_media_pipeline_t * mediaPipeline() const = 0;

    // TODO: this might be removed in the future..
    virtual void configured() {};
};

/**
 * MediaPlatformPrivate is an interface for functions that should be handled by
 * the decoder orchestrator, this includes things like starting/stopping
 * a clock or decoders, querying the last rendered frame time.
 */
class MediaPlatformPrivate : public RefCounted<MediaPlatformPrivate> {
public:
    MediaPlatformPrivate() { }
    virtual ~MediaPlatformPrivate() { }

    // TODO: clock source??
    virtual void connectPipeline(VHSMediaPipeline *pipeline) const = 0;

    // Once start is called on the pipeline, the buffer_available callbacks will
    // occur. The buffer with clock source set to true will be called last.
    //void start(); // TODO: play() ??

    virtual uint64_t lastTimeStamp() const = 0;

};

}

#endif // MediaPlatformPrivate_h
