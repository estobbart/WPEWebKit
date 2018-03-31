#include "config.h"
#include "MediaSampleVHS.h"

#include <wtf/PrintStream.h>
#include <math.h>

#include <vhs/avc.h> // vhs_param_sets_t
#include <vhs/aac.h> // vhs_aac_config_t

namespace WebCore {

// MetaMediaSampleVHS is used to hold the non-display copy of a sample
class MetaMediaSampleVHS final : public MediaSample {
public:
    static Ref<MetaMediaSampleVHS> create(MediaTime presentationTime, MediaTime decodeTime) {
        return adoptRef(*new MetaMediaSampleVHS(presentationTime, decodeTime));
    }

private:
    MetaMediaSampleVHS(MediaTime presentationTime, MediaTime decodeTime)
    : m_presetationTime(presentationTime)
    , m_decodeTime(decodeTime) {

    }

    virtual ~MetaMediaSampleVHS() { }

    MediaTime presentationTime() const override { return m_presetationTime; }
    MediaTime decodeTime() const override { return m_decodeTime; }

    // The remaining values are not used in non-display copies
    MediaTime duration() const override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return MediaTime();
    }

    AtomicString trackID() const override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return nullAtom();
    }

    void setTrackID(const String&) override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
    }

    size_t sizeInBytes() const override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return 0;
    }

    FloatSize presentationSize() const override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return FloatSize(0, 0);
    }

    void offsetTimestampsBy(const MediaTime&) override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
    }

    void setTimestamps(const MediaTime&, const MediaTime&) override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
    }

    bool isDivisable() const override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return false;
    }

    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime&) override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return { nullptr, nullptr };
    }

    Ref<MediaSample> createNonDisplayingCopy() const override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return MetaMediaSampleVHS::create(m_presetationTime, m_decodeTime);
    }

    SampleFlags flags() const override {
        printf("MetaMediaSampleVHS%s\n", __FUNCTION__);
        return MediaSample::IsNonDisplaying;
    }

    PlatformSample platformSample() override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
        return PlatformSample();
    }

    void dump(PrintStream&) const override {
        printf("Error MetaMediaSampleVHS%s\n", __FUNCTION__);
    }

    MediaTime m_presetationTime;
    MediaTime m_decodeTime;

};

uint32_t MediaSampleVHS::sizeOfNextPESPacket() {
    vhs_node_t *box = vhs_node_child(m_segment->root(), "trun");
    vhs_trun_box_t *trun = box ? VHS_TRUN_BOX(vhs_node_raw(box)) : NULL;
    if (trun) {
        vhs_sample_t *sample = vhs_trun_sample_peek(trun, &m_cursor);
        if (sample) {
            // 14 byte PES header
            uint32_t pesSize = 14;
            pesSize += vhs_sample_size(sample);
            if (m_mediaDefaults->sampleType() == VHSAudioSampleType) {
                // 7 byte ADTS header,
                pesSize += 7;
            } else if (m_mediaDefaults->sampleType() == VHSVideoSampleType && vhs_sample_depends_on(sample) == vhs_sample_depends_false) {
                // printf("vhs_sample_depends_false\n");

                vhs_param_sets_t *paramSets = (vhs_param_sets_t *)m_mediaDefaults->decoderConfig();
                pesSize += paramSets->sps[0]->nal_len + 4;
                pesSize += paramSets->pps[0]->nal_len + 4;
                // TODO: We don't support nal_length != 4
                // Video only has an additional size if the nal_length
                // isn't 4 bytes & it's not an IFrame.

            }
            return pesSize;
        }
    }
    return 0;
}

// 2112 / 1024 (samples per frame) = 2.0625 * frame duration (90k???)
// static const uint16_t AACEncoderSampleDelay = 2112;

void MediaSampleVHS::writeNextPESPacket(uint8_t *buffer, size_t *size) {
    // if (isEncrypted()) {
    //     printf("MediaSampleVHS::writeNextPESPacket ERROR buffer is encrypted\n");
    // }

    *size = 0;

    vhs_node_t *box = vhs_node_child(m_segment->root(), "trun");
    vhs_trun_box_t *trun = box ? VHS_TRUN_BOX(vhs_node_raw(box)) : NULL;

    uint8_t *mdatData = NULL;
    // TODO: check the mdatSize to make sure we're not reading beyond the mdat
    size_t mdatSize = 0;
    sampleData(&mdatData, &mdatSize);
    if (!mdatData || !mdatSize) {
        return;
    }

    box = vhs_node_child(m_segment->root(), "tfdt");
    vhs_tfdt_box_t *tfdt = box ? VHS_TFDT_BOX(vhs_node_raw(box)) : NULL;

    if (trun && tfdt && m_cursor) {
        vhs_sample_t *sample = vhs_trun_sample_next(trun, &m_cursor);
        if (sample) {

            // TODO: need to check this across stream sources..
            // NOTE: AAC encoders have a general delay of 2112 samples a little
            // more than 2 access units (1024 samples per access unit)
            // 2112 / 24KHz rate * 90k time base = 7920
            // if (m_mediaDefaults->sampleType() == VHSAudioSampleType && m_ptsAccumulation == 0) {
            //     m_ptsAccumulation += 22500; // previous used 22000 .38 ms - 3 frames
            // }
            // TODO: Need to properly handle rollovers
            // TODO: convert all of these in to a 90k clock.
            uint64_t pts = m_ptsAccumulation + vhs_tfdt_base_media_decode_time(tfdt) + m_timestampOffset;
            printf("writeNextPESPacket() -- PES pts: %llu\n", pts);
            uint32_t sampleSize = vhs_sample_size(sample);
            uint32_t headerSize = 0;
            if (m_mediaDefaults->sampleType() == VHSAudioSampleType) {
                // 7 byte ADTS header,
                headerSize += 7;
            } else if (m_mediaDefaults->sampleType() == VHSVideoSampleType) {
                // TODO: Move this condition into the else if
                if (vhs_sample_depends_on(sample) == vhs_sample_depends_false) {

                    // decoder config nal_length
                    // avcC from the init header..

                    // We don't consider this part of the header.
                    // the header would be if the nal_length didn't match
                    // the start_code 4 bytes, which is a a TODO to support
                    vhs_param_sets_t *paramSets = (vhs_param_sets_t *)m_mediaDefaults->decoderConfig();
                    sampleSize += paramSets->sps[0]->nal_len + 4;
                    sampleSize += paramSets->pps[0]->nal_len + 4;

                }

                // TODO: We don't support nal_length != 4
                // Video only has an additional size if the nal_length
                // isn't 4 bytes & it's not an IFrame.
                // TODO: add a printf error here if the nal_length != 4

            } else {
                printf("ERROR: MediaSampleVHS::writeNextPESPacket !m_hasAudio && !m_hasVideo\n");
                return;
            }

            vhs_pes_set_size(m_pesPacket, sampleSize + headerSize);
            vhs_pes_set_pts(m_pesPacket, pts);

            uint8_t *pesBuffer = NULL;
            size_t pesSize = 0;
            vhs_pes_raw(m_pesPacket, &pesBuffer, &pesSize);

            memcpy(buffer, pesBuffer, pesSize);
            buffer += pesSize;
            *size += pesSize;

            // This sampleSize value is slightly confusing.. it's used to
            // write the ESStream packet size, then reassigned to be able to
            // do the MDAT memcopy & move the readOffset.
            // TODO: sample size might also be in the defaults, need to provide
            // a util method similar to `sampleDuration(sample)`.
            sampleSize = vhs_sample_size(sample);

            // printf("m_ptsAccumulation adding : %u\n", sampleDuration(sample));
            if (m_mediaDefaults->mediaTimescale() != 90000) {
                printf("Not 90k.. converting\n");
                m_ptsAccumulation += (sampleDuration(sample) * 90000) / m_mediaDefaults->mediaTimescale();
            } else {
                m_ptsAccumulation += sampleDuration(sample);
            }

            if (m_mediaDefaults->sampleType() == VHSAudioSampleType) {
                vhs_aac_config_t *aacConf = (vhs_aac_config_t *)m_mediaDefaults->decoderConfig();
                // TODO: does this need the address of the buffer pointer??
                vhs_aac_adts_header_write(aacConf, sampleSize, &buffer);
                buffer += headerSize;
                *size += headerSize;

                memcpy(buffer, &mdatData[m_mdatReadOffset], sampleSize);
                *size += sampleSize;
                m_mdatReadOffset += sampleSize;

                return;

            } else if (m_mediaDefaults->sampleType() == VHSVideoSampleType) {
                vhs_param_sets_t *paramSets = NULL;
                if (vhs_sample_depends_on(sample) == vhs_sample_depends_false) {
                  paramSets = (vhs_param_sets_t *)m_mediaDefaults->decoderConfig();
                }
                // TODO does this need the address of the buffer pointer?
                vhs_avc_write_frame_to_annex_b(paramSets,
                                               3, // TODO: hardcoded length - 1
                                               &mdatData[m_mdatReadOffset],
                                               sampleSize,
                                               &buffer);
                if (paramSets) {
                    *size += paramSets->sps[0]->nal_len + 4;
                    *size += paramSets->pps[0]->nal_len + 4;
                }
                *size += sampleSize;
                m_mdatReadOffset += sampleSize;

                return;

            }
        } else {
            printf("MediaSampleVHS::writeNextPESPacket no more samples\n");
            return;
        }
    } else {
        printf("ERROR: MediaSampleVHS::writeNextPESPacket !trun || !mdatData || !tfdt || !m_cursor\n");
    }
    printf("ERROR: MediaSampleVHS::writeNextPESPacket REACH THE END WITHOUT A RETURN!\n");
    return;

}

void MediaSampleVHS::sampleData(uint8_t **buffer, size_t *size) const {
    vhs_node_t *node = vhs_node_child(m_segment->root(), "mdat");
    if (!node) {
        printf("ERROR: MediaSampleVHS::sampleData NULL, 0 %zu \n", *size);
        *buffer = NULL;
        *size = 0;
        return;
    }
    vhs_mdat_box_t *mdat = VHS_MDAT_BOX(vhs_node_raw(node));
    *buffer = vhs_mdat_data(mdat);
    *size = vhs_mdat_data_size(mdat);
    // if (m_mdatReadOffset == 0) {
    //     printf("MediaSampleVHS::sampleBuffer ----> :%zu \n", *size);
    // }
}

uint32_t MediaSampleVHS::sampleDuration(vhs_sample_t *sample) const {
    uint32_t sample_duration = vhs_sample_duration(sample);
    if (!sample_duration) {
        if (!m_sample_duration_default) {
            vhs_node_t *box = vhs_node_child(m_segment->root(), "tfhd");
            vhs_tfhd_box_t *tfhd = box ? VHS_TFHD_BOX(vhs_node_raw(box)) : NULL;
            if (tfhd) {
                m_sample_duration_default = vhs_tfhd_sample_duration_default(tfhd);
            }
        }
        sample_duration = m_sample_duration_default;
    }
    return sample_duration;
}

MediaTime MediaSampleVHS::presentationTime() const {
    if (m_presentationTime.isInvalid()) {
        vhs_node_t *box = vhs_node_child(m_segment->root(), "trun");
        vhs_trun_box_t *trun = box ? VHS_TRUN_BOX(vhs_node_raw(box)) : NULL;
        if (trun) {
            MediaTime _decodeTime = this->decodeTime();
            printf("_decodeTime.toDouble() %f m_mediaDefaults->mediaTimescale() %u\n", _decodeTime.toDouble(), m_mediaDefaults->mediaTimescale());
            double pts = vhs_trun_first_presentation_time(trun, _decodeTime.toDouble(), m_mediaDefaults->mediaTimescale());
            printf("%s %f\n", __PRETTY_FUNCTION__, pts);
            m_presentationTime = MediaTime::createWithDouble(pts);
        }
    }
    return m_presentationTime + MediaTime(m_timestampOffset, m_mediaDefaults->mediaTimescale());
}

MediaTime MediaSampleVHS::decodeTime() const {
    if (m_decodeTime.isInvalid()) {
        vhs_node_t *box = vhs_node_child(m_segment->root(), "tfdt");
        vhs_tfdt_box_t *tfdt = box ? VHS_TFDT_BOX(vhs_node_raw(box)) : NULL;
        if (tfdt) {
            // printf("decodeTime(): BASE MEDIA DECODE TIME:%llu\n", vhs_tfdt_base_media_decode_time(tfdt));
            double dts = vhs_tfdt_decode_time_seconds(tfdt, m_mediaDefaults->mediaTimescale());
            printf("%s %f\n", __PRETTY_FUNCTION__, dts);
            m_decodeTime = MediaTime::createWithDouble(dts);
        }
    }
    return m_decodeTime + MediaTime(m_timestampOffset, m_mediaDefaults->mediaTimescale());
}

// TODO: use the sampleDuration() to help determine the sum of the media sample
// duration.
MediaTime MediaSampleVHS::duration() const {
    if (m_duration.isInvalid()) {
        vhs_node_t *box = vhs_node_child(m_segment->root(), "trun");
        vhs_trun_box_t *trun = box ? VHS_TRUN_BOX(vhs_node_raw(box)) : NULL;
        if (trun) {
            vhs_cursor_t *cursor = vhs_cursor_init();
            vhs_sample_t *sample = vhs_trun_sample_next(trun, &cursor);
            if (vhs_sample_duration(sample)) {
                // TODO: Move the timescale to here..
                double duration = vhs_trun_duration(trun, m_mediaDefaults->mediaTimescale());
                printf("%s %f\n", __PRETTY_FUNCTION__, duration);
                m_duration = MediaTime::createWithDouble(duration);
            } else {
                box = vhs_node_child(m_segment->root(), "tfhd");
                vhs_tfhd_box_t *tfhd = box ? VHS_TFHD_BOX(vhs_node_raw(box)) : NULL;
                uint32_t durationPerSample = 0;
                if (tfhd) {
                    durationPerSample = vhs_tfhd_sample_duration_default(tfhd);
                }
                if (!durationPerSample) {
                    durationPerSample = m_mediaDefaults->sampleDuration();
                }
                m_duration = MediaTime((int64_t)(durationPerSample * vhs_trun_sample_count(trun)),
                                       m_mediaDefaults->mediaTimescale());
                printf("%s %f\n", __PRETTY_FUNCTION__, m_duration.toDouble());
            }
        }
    }
    return m_duration;
}

size_t MediaSampleVHS::sizeInBytes() const {
    vhs_node_t *box = vhs_node_child(m_segment->root(), "mdat");
    vhs_mdat_box_t *mdat = box ? VHS_MDAT_BOX(vhs_node_raw(box)) : NULL;
    size_t size = 0;
    if (mdat) {
        size = vhs_mdat_data_size(mdat);
        printf("%s %zu\n", __PRETTY_FUNCTION__, size);
    }
    return size;
}

FloatSize MediaSampleVHS::presentationSize() const {
    printf("%s\n", __PRETTY_FUNCTION__);
    return m_mediaDefaults->trackPresentationSize();
}

/**
 * TODO: requires more research why this is even exposed or needed.
 */
void MediaSampleVHS::setTrackID(const String& id) {
    printf("ERROR MediaSampleVHS::setTrackID %s NOT SUPPORTED", id.utf8().data());
    printf("... current id %s \n", m_id.string().utf8().data());
}

void MediaSampleVHS::offsetTimestampsBy(const MediaTime &mediaTime) {
    printf("MediaSampleVHS::offsetTimestampsBy %f\n", mediaTime.toFloat());
    m_timestampOffset = round(mediaTime.toDouble() * m_mediaDefaults->mediaTimescale());
}

// TODO: It's not clear when/why this would be called.
void MediaSampleVHS::setTimestamps(const MediaTime &mediaTimeOne __attribute__((unused)),
                                     const MediaTime &mediaTimeTwo __attribute__((unused))) {
  printf("ERROR TODO: MediaSampleVHS::setTimestamps\n");
}

// NOTE(estobb200): For video a sample would only be divisible if it had multiple
// keyframes in it that could be used as RAPs. Since in S8 the keyframes are
// only at the start of a sample it's not worth dividing them. We do need to
// support an iframe track, and in that case would get multiple keyframes in
// a sample. But it still TDB if it's worth dividing them.
// TODO: For audio we can divide while the sample count is > 1.
bool MediaSampleVHS::isDivisable() const {
    printf("ERROR TODO: MediaSampleVHS::isDivisable\n");
    return false;
}

std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> MediaSampleVHS::divide(const MediaTime& presentationTime __attribute__((unused))) {
    printf("ERROR TODO: MediaSampleVHS::divide\n");
    return { nullptr, nullptr };
}

// TODO: Create a non-display copy by freeing the MDAT box
// means having a ref counted vhs_node_t* also :(
Ref<MediaSample> MediaSampleVHS::createNonDisplayingCopy() const {
    printf("MediaSampleVHS::createNonDisplayingCopy -> MetaMediaSampleVHS\n");
    return MetaMediaSampleVHS::create(presentationTime(), decodeTime());
}

// NOTE(estobb200): Every sample from S8 is considered a sync sample,
// See comments about being divisible.
MediaSample::SampleFlags MediaSampleVHS::flags() const {
    // TODO: What does audio look like here?
    printf("ERROR TODO: MediaSampleVHS::flags\n");
    /*
    enum SampleFlags {
        None = 0,
        IsSync = 1 << 0,
        IsNonDisplaying = 1 << 1,
    };
    */
    return MediaSample::IsSync;
}

// TODO: What does this really mean?
PlatformSample MediaSampleVHS::platformSample() {
    printf("ERROR TODO: MediaSampleVHS::platformSample\n");
    return PlatformSample();
}

void MediaSampleVHS::dump(PrintStream &out) const {
    out.print("{PTS(", presentationTime(), "), DTS(", decodeTime(), "), duration(", duration(), "), flags(", (int)flags(), "), presentationSize(", presentationSize().width(), "x", presentationSize().height(), ")}");
}

}
