#ifndef MediaSampleHelio_H
#define MediaSampleHelio_H

#include "MediaSample.h"

#include <stdio.h>
#include <string.h>

#include "rcvmf_isobmff.h"
#include "pes.h"
#include "aac.h"
#include "avc.h"

// TODO: Read up on this..

// https://developer.apple.com/documentation/coremedia/cmsamplebuffer?language=objc

namespace WebCore {

typedef enum HelioCodecType {
    HelioCodecTypeUnknown,
    HelioCodecTypeAudio,
    HelioCodecTypeVideo
} HelioCodecType;

/**
 * HelioCodecConfiguration holds the decoder initialization data per MediaSample
 *
 * this is a convenience to use WebKit's ref counting to pass this object around
 * and let WebKit handle it's destruction
 */
class HelioCodecConfiguration : public RefCounted<HelioCodecConfiguration> {
public:
    HelioCodecConfiguration() { }
    virtual ~HelioCodecConfiguration() { }

    virtual HelioCodecType codecType() const { return HelioCodecTypeUnknown; }

    virtual void * decoderConfiguartion() const = 0;
};

class HelioAudioCodecConfiguration final : public HelioCodecConfiguration {
public:
    static Ref<HelioAudioCodecConfiguration> create(uint8_t profile,
                                                    uint8_t sampling_frequency_index,
                                                    uint8_t channel_config) {
        printf("HelioAudioCodecConfiguration::create\n");
        return adoptRef(*new HelioAudioCodecConfiguration(profile,
                                                          sampling_frequency_index,
                                                          channel_config));
    }

    ~HelioAudioCodecConfiguration() {
        printf("HelioAudioCodecConfiguration ~HelioAudioCodecConfiguration\n");
        free(m_aacConf);
    }

    HelioCodecType codecType() const { return HelioCodecTypeAudio; };

    rcv_aac_config_t * decoderConfiguartion() const override { return m_aacConf; }

private:
    HelioAudioCodecConfiguration(uint8_t profile,
                                 uint8_t sampling_frequency_index,
                                 uint8_t channel_config) {
        printf("HelioAudioCodecConfiguration profile:%u sampling_frequency_index:%u channel_config:%u\n",
               profile, sampling_frequency_index, channel_config);
        m_aacConf = malloc(sizeof(rcv_aac_config_t));
        memset(m_aacConf, 0, sizeof(rcv_aac_config_t));
        m_aacConf->profile = profile;
        m_aacConf->sampling_frequency_index = sampling_frequency_index;
        m_aacConf->channel_config = channel_config;
    }

    rcv_aac_config_t *m_aacConf;
};

class HelioVideoCodecConfiguration final : public HelioCodecConfiguration {
public:
    static Ref<HelioVideoCodecConfiguration> create(rcv_param_sets_t *paramSets) {
        printf("HelioVideoCodecConfiguration::create\n");
        return adoptRef(*new HelioVideoCodecConfiguration(paramSets));
    }

    ~HelioVideoCodecConfiguration() {
        printf("HelioVideoCodecConfiguration ~HelioVideoCodecConfiguration\n");
        // TODO: This is a mess.. refactor needed.
        free(m_paramSets->sps[0]->nal_unit);
        free(m_paramSets->pps[0]->nal_unit);

        free(m_paramSets->sps[0]);
        free(m_paramSets->sps[1]);

        free(m_paramSets->pps[0]);
        free(m_paramSets->pps[1]);

        free(m_paramSets->sps);
        free(m_paramSets->pps);

        free(m_paramSets);
    }

    HelioCodecType codecType() const { return HelioCodecTypeVideo; };

    rcv_param_sets_t * decoderConfiguartion() const override { return m_paramSets; }

private:
    HelioVideoCodecConfiguration(rcv_param_sets_t *paramSets) {
        m_paramSets = paramSets;
    }

    rcv_param_sets_t *m_paramSets;
};

/**
 * Stores an ISOBMFF Media Segment, and an out of band codec configuration.
 *
 * A MediaSampleHelio object and a media segment (w3c) are 1 to 1.
 *
 * https://www.w3.org/TR/mse-byte-stream-format-isobmff/
 *
 * codec configuration for H264 would be the avc1 box.
 */
class MediaSampleHelio final : public MediaSample {

public:
    /**
     * MediaSampleHelio owns the rcv_node_t *
     */
    static Ref<MediaSampleHelio> create(rcv_node_t *root,
                                        uint32_t timescale,
                                        RefPtr<HelioCodecConfiguration>& codecConf) {
        return adoptRef(*new MediaSampleHelio(root,
                                              timescale,
                                              codecConf)); // TODO: WTF::move??
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
     * The size_t* arg is optional by passing NULL.
     *
     * TODO: Move the decrypt function out of here..
     */
    bool writeNextPESPacket(uint8_t **buffer, size_t *size);


    bool isEncrypted() const;

    bool decryptBuffer(std::function<int(const void* iv, uint32_t ivSize, void* payloadData, uint32_t payloadDataSize)> decrypt);

    /**
     * TODO: This will eventually be needed when we need to decrypt the content
     */
    void sampleBuffer(uint8_t **buffer, size_t *size);
//    void * box(char * fourcc);

    // TODO: Why was this private?
    AtomicString trackID() const override {
        return m_id;
    }

private:
    MediaSampleHelio(rcv_node_t *root, uint32_t timescale, RefPtr<HelioCodecConfiguration>& codecConf)
      : m_presentationTime()
      , m_decodeTime()
      , m_duration()
      , m_timestampOffset(0) {
        m_sample = root;
        m_codecConf = codecConf; // TODO: WTF::move??
        m_cursor = rcv_cursor_init();
        m_mdatReadOffset = 0;
        m_ptsAccumulation = 0;
        m_encryptedBuffer = rcv_node_child(m_sample, "senc") != NULL;

        rcv_node_t *box = rcv_node_child(m_sample, "tfhd");
        if (box) {
            rcv_tfhd_box_t *tfhd = RCV_TFHD_BOX(rcv_node_raw(box));
            m_id = AtomicString::number(rcv_tfhd_track_id(tfhd));
        } else {
            printf("ERROR: MediaSampleHelio UNABLE TO GET rcv_tkhd_track_id\n");
            m_id = AtomicString::number(0);
        }

        m_hasVideo = m_codecConf->codecType() == HelioCodecTypeVideo;
        m_hasAudio = m_codecConf->codecType() == HelioCodecTypeAudio;

          // This is only in the init segment
//        box = rcv_node_child(m_sample, "hdlr");
//        if (box) {
//            rcv_hdlr_box_t *hdlr = RCV_HDLR_BOX(rcv_node_raw(box));
//            char *hdlr_type =  rcv_hdlr_handler_type(hdlr);
//            if (strcmp(hdlr_type, "soun") == 0) {
//                m_hasAudio = true;
//            } else if (strcmp(hdlr_type, "vide") == 0) {
//                m_hasVideo = true;
//            } else {
//                printf("ERROR: MediaSampleHelio hdlr_type:%s\n", hdlr_type);
//            }
//
//        } else {
//            printf("ERROR: MediaSampleHelio NO HDLR BOX\n");
//        }
//        if (!m_hasVideo && !m_hasAudio) {
//            printf("ERROR: MediaSampleHelio has neither Audio or Video\n");
//        }

        m_pesPacket = cvmf_pes_create(m_hasVideo ? cvmf_pes_type_video : (m_hasAudio ? cvmf_pes_type_audio : cvmf_pes_type_unknown) );

        m_timescale = timescale;
        //printf("MediaSampleHelio m_timescale %u\n", m_timescale);
    }

    virtual ~MediaSampleHelio() {
        printf("~MediaSampleHelio\n");
        if (m_sample) {
            rcv_destory_tree(&m_sample);
        }
//        if (m_codecConf) {
//            rcv_destory_tree(&m_codecConf);
//        }
        if (m_cursor) {
            rcv_cursor_destroy(&m_cursor);
        }
        if (m_pesPacket) {
            cvmf_pes_destroy(&m_pesPacket);
        }
    }

    MediaTime presentationTime() const override;
    MediaTime decodeTime() const override;
    MediaTime duration() const override;

    // TODO: TFHD is it's track ID, we shouldn't support set here.
    // TODO: Why is this part of the interface?
    void setTrackID(const String& id) override {
        printf("ERROR MediaSampleHelio::setTrackID %s NOT SUPPORTED", id.utf8().data());
        printf("... current id %s \n", m_id.string().utf8().data());
    }

    /**
     * size in bytes returns the size of the MDAT box.
     */
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
    rcv_cursor_t *m_cursor; // Is this the mdat cursor?
    cvmf_pes_t *m_pesPacket;
    uint64_t m_mdatReadOffset;
    int64_t m_ptsAccumulation;

    uint32_t m_timescale;

    bool m_hasAudio;
    bool m_hasVideo;

    RefPtr<HelioCodecConfiguration> m_codecConf;

    int64_t m_timestampOffset;

    // Cached values.
    // TODO: Capture these values when a non-display copy is needed.
    MediaTime m_presentationTime;
    MediaTime m_decodeTime;
    MediaTime m_duration;

    bool m_encryptedBuffer;
};

}

#endif
