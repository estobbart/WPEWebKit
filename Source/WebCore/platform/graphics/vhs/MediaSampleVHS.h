#ifndef MediaSampleVHS_h
#define MediaSampleVHS_h

#include "MediaSample.h"
#include "MediaSegmentVHS.h"
#include <vhs/vhs_isobmff.h>
#include <vhs/pes.h>

namespace WebCore {

enum VHSSampleType {
    VHSUnknownSampleType = 0,
    VHSAudioSampleType = 1,
    VHSVideoSampleType = 1 << 1
};

/**
 * A MediaSample could be as simple as a moof & mdat, according to the isobmff
 * byte stream format spec.
 * https://www.w3.org/TR/mse-byte-stream-format-isobmff/
 * It's possible that a moof box may refer back to defaults that exist in the
 * moov box that is part of the initialization segment. Instead of providing
 * those defaults to each sample, the sample will determine if they're needed
 * and query back to the MediaSampleDefaultsProxy
 */
class MediaSampleVHSDefaultsProxy : public RefCounted<MediaSampleVHSDefaultsProxy> {
public:
    virtual ~MediaSampleVHSDefaultsProxy() { }

    /**
     * timescale from the mdhd, sampleDuration divided by mediaTimescale
     * determines the number of ms duration per sample.
     *
     * NOTE(estobbart): This is not the same as the moov timescale, which
     * applies to the overall content. This is specific to the trak->mdia->mdhd.
     */
    virtual uint32_t mediaTimescale() const = 0;

    /**
     * default values from either the tkhd or trex
     */
    virtual uint64_t sampleDuration() const = 0;

    /**
     * If track (trak) has a width & height, else FloatSize(0, 0).
     */
    virtual FloatSize trackPresentationSize() const = 0;

    /**
     * returns the sample type flags.
     * TODO: may be better to pass an AtomicString here for the trackId
     */
    virtual VHSSampleType sampleType() const = 0;

    /**
     * returns a pointer to the decoder configuration, safe to cast based on
     * the sampleType.
     * TODO: make a union of the types
     */
    virtual void * decoderConfig() const = 0;
};

/**
 * MediaSampleVHS wraps a vhs_node_t which should be the root of an isobmff
 * MediaSegment.
 * It requires the following boxes:
 *  a tfhd for the trackID
 *  a trun for the presentationTime & duration
 *  a tfdt for the decodeTime
 *  a mdat for the sizeInBytes
 */
class MediaSampleVHS final : public MediaSample {

public:
    static Ref<MediaSampleVHS> create(Ref<MediaSegmentVHS>&& segment, Ref<MediaSampleVHSDefaultsProxy>&& mediaDefaults) {
        return adoptRef(*new MediaSampleVHS(WTFMove(segment), WTFMove(mediaDefaults)));
    }

    /**
     * this is not an enumeration, it is a look ahead to determine if the buffer
     * being written to contains enough space for the next packet.
     * When the packets in this sample have all been written, this function
     * returns 0.
     */
    uint32_t sizeOfNextPESPacket();

    /**
     * A buffer with space larger than the current sizeOfNextPESPacket value
     * If the sample cursor is exhausted, no data will be written to the buffer
     * and the return value will be false.
     * The size_t* arg is not optional, it will be assigned zero, and then
     * incremented as memcpy's occur to the buffer.
     */
    void writeNextPESPacket(uint8_t *buffer, size_t *size);

// TODO: What does this look like in a constructor (see SourceBuffer)
private:
    MediaSampleVHS(Ref<MediaSegmentVHS>&& segment, Ref<MediaSampleVHSDefaultsProxy>&& mediaDefaults)
    : m_segment(WTFMove(segment))
    , m_mediaDefaults(WTFMove(mediaDefaults))
    , m_timestampOffset(0)
    , m_ptsAccumulation(0)
    , m_mdatReadOffset(0)
    , m_sample_duration_default(0)
    , m_presentationTime(MediaTime::invalidTime())
    , m_decodeTime(MediaTime::invalidTime())
    , m_duration(MediaTime::invalidTime()) {
        vhs_node_t *box = vhs_node_child(m_segment->root(), "tfhd");
        vhs_tfhd_box_t *tfhd = box ? VHS_TFHD_BOX(vhs_node_raw(box)) : NULL;
        if (tfhd) {
            printf("%s vhs_tfhd_track_id:%u\n", __PRETTY_FUNCTION__, vhs_tfhd_track_id(tfhd));
            m_id = AtomicString::number(vhs_tfhd_track_id(tfhd));
        } else {
            // TODO: get the track id from the mediaDefaults
            printf("ERROR: MediaSampleVHS UNABLE TO GET vhs_tfhd_track_id\n");
            m_id = AtomicString::number(0);
        }
        m_cursor = vhs_cursor_init();

        VHSSampleType vhsSampleType = m_mediaDefaults->sampleType();
        vhs_pes_type_t sampleType = vhs_pes_type_unknown;
        if (vhsSampleType == VHSVideoSampleType) {
            sampleType = vhs_pes_type_video;
        } else if (vhsSampleType == VHSAudioSampleType) {
            sampleType = vhs_pes_type_audio;
        }
        m_pesPacket = vhs_pes_create(sampleType);
    }

    virtual ~MediaSampleVHS() {
        if (m_cursor) {
            vhs_cursor_destroy(&m_cursor);
        }
        if (m_pesPacket) {
            vhs_pes_destroy(&m_pesPacket);
        }
    }

    /**
     * presentationTime, decodeTime, & duration are calculated from the first
     * moof box. If a MediaSample contains multiple moof boxes, it should
     * first be split (see MediaSegmentVHS) into multiple MediaSampleVHSs.
     *
     */
    MediaTime presentationTime() const override;
    MediaTime decodeTime() const override;
    /**
     * duration() is the sum of sampleDuration() for all the trun samples.
     *
     * Each AAC access unit or AVC frame is also known as a sample, and those
     * individual sample durations can be found using the `sampleDuration`
     * method.
     */
    MediaTime duration() const override;
    AtomicString trackID() const override { return m_id;}
    void setTrackID(const String&) override;
    size_t sizeInBytes() const override;
    FloatSize presentationSize() const override;
    void offsetTimestampsBy(const MediaTime&) override;
    void setTimestamps(const MediaTime&, const MediaTime&) override;
    bool isDivisable() const override;
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime) override;
    Ref<MediaSample> createNonDisplayingCopy() const override;
    SampleFlags flags() const override;
    PlatformSample platformSample() override;
    void dump(PrintStream&) const override;

    /**
     * assigns the mdat data pointer & mdat data size to the pointers provided
     */
    void sampleData(uint8_t **buffer, size_t *size) const;
    uint32_t sampleDuration(vhs_sample_t *sample) const;

    Ref<MediaSegmentVHS> m_segment;
    Ref<MediaSampleVHSDefaultsProxy> m_mediaDefaults;

    AtomicString m_id;
    int64_t m_timestampOffset;

    // Cursor for the position in the trun samples when writing out PES packets
    // It is allocated and initialized during sample create, and iterated
    // and free'd during the sample write. Or if the sample is discarded,
    // it's free'd in the destructor.
    vhs_cursor_t *m_cursor;
    vhs_pes_t *m_pesPacket;

    int64_t m_ptsAccumulation;
    uint64_t m_mdatReadOffset;

    mutable uint32_t m_sample_duration_default;

    // TODO: is this Audio or Video??

    // Cached values.
    // TODO: Capture these values when a non-display copy is needed.
    mutable MediaTime m_presentationTime;
    mutable MediaTime m_decodeTime;
    mutable MediaTime m_duration;
};

} // webcore

#endif
