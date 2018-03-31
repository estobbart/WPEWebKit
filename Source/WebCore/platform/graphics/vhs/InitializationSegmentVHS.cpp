#include "config.h"
#include "InitializationSegmentVHS.h"
#include <vhs/vhs_isobmff.h>

namespace WebCore {

/**
 * InitializationSegmentVHS is the ftyp & moof boxes of an ISOBMFF byte stream
 */
InitializationSegmentVHS::InitializationSegmentVHS(Ref<MediaSegmentVHS>&& segment)
  : m_segment(WTFMove(segment))
  , m_timescale(0)
  , m_duration(0)
  , m_trackId(0) {

    m_audioConfig = NULL;
    m_videoConfig = NULL;

    // Video configuration
    vhs_node_t *node = vhs_node_child(m_segment->root(), "avcC");
    vhs_avcC_box_t *avcC = node ? VHS_AVCC_BOX(vhs_node_raw(node)) : NULL;
    if (avcC) {
        if (vhs_avcC_nal_unit_length(avcC) != 4) {
            // This isn't supported yet when writing the stream..
            printf("ERROR: avcC nal_length:%u\n", vhs_avcC_nal_unit_length(avcC));
        }
        vhs_cursor_t *cursor = vhs_cursor_init();

        vhs_nal_param_t **sps = (vhs_nal_param_t **)malloc(sizeof(vhs_nal_param_t *) * 2);
        sps[0] = (vhs_nal_param_t *)malloc(sizeof(vhs_nal_param_t));
        // TODO: Can this be a NULL pointer? or does it require additional mallocd memory?
        sps[1] = (vhs_nal_param_t *)malloc(sizeof(vhs_nal_param_t));
        sps[1]->nal_unit = NULL;
        sps[1]->nal_len = 0;


        vhs_nal_param_t **pps = (vhs_nal_param_t **)malloc(sizeof(vhs_nal_param_t *) * 2);
        pps[0] = (vhs_nal_param_t *)malloc(sizeof(vhs_nal_param_t));
        pps[1] = (vhs_nal_param_t *)malloc(sizeof(vhs_nal_param_t));
        pps[1]->nal_unit = NULL;
        pps[1]->nal_len = 0;

        // TODO: Make sure cursor is free'd
        vhs_avcC_parameter_set_t *param_set = vhs_avcC_sps_next(avcC, &cursor);
        vhs_avcC_parameter_set_assign_and_free(&param_set, &sps[0]->nal_unit, &sps[0]->nal_len);
        // if (cursor != NULL) {
        //     // TODO: This might mean we actually have to call next a second time
        //     // this may be a bug in the cursor code
        //     printf("ERROR sps_cursor not freed\n");
        // }
        // if (param_set != NULL) {
        //     printf("ERROR param_set not freed\n");
        // }

        cursor = vhs_cursor_init();
        param_set = vhs_avcC_pps_next(avcC, &cursor);
        vhs_avcC_parameter_set_assign_and_free(&param_set, &pps[0]->nal_unit, &pps[0]->nal_len);
        // if (cursor != NULL) {
        //     // TODO: See above.
        //     printf("ERROR sps_cursor not freed\n");
        // }
        // if (param_set != NULL) {
        //     printf("ERROR param_set not freed\n");
        // }

        m_videoConfig = (vhs_param_sets_t*)malloc(sizeof(vhs_param_sets_t));
        m_videoConfig->sps = sps;
        m_videoConfig->pps = pps;
    }

    // Audio configuration
    node = vhs_node_child(m_segment->root(), "esds");
    vhs_esds_box_t *esds = node ? VHS_ESDS_BOX(vhs_node_raw(node)) : NULL;
    if (esds) {
        m_audioConfig = (vhs_aac_config_t *)malloc(sizeof(vhs_aac_config_t));
        vhs_decoder_specific_info_t *dsi = vhs_esds_decoder_specifc_info(esds);
        if (dsi) {
            m_audioConfig->profile = vhs_esds_dsi_audio_object_type(dsi);
            m_audioConfig->sampling_frequency_index = vhs_esds_dsi_sampling_frequency_index(dsi);
            m_audioConfig->channel_config = vhs_esds_dsi_channel_config(dsi);
        } else {
            printf("ERROR: NO DSI\n");
            memset(m_audioConfig, 0, sizeof(vhs_aac_config_t));
        }
    }
}

uint32_t InitializationSegmentVHS::timescale() const {
    if (!m_timescale) {
        vhs_node_t *box = vhs_node_child(m_segment->root(), "mvhd");
        vhs_mvhd_box_t *mvhd = box ? VHS_MVHD_BOX(vhs_node_raw(box)) : NULL;
        if (mvhd) {
            m_timescale = vhs_mvhd_timescale(mvhd);
        }
    }
    return m_timescale;
}

uint64_t InitializationSegmentVHS::duration() const {
    if (!m_duration) {
        // mehd is an optional box that may include duration information
        vhs_node_t *box = vhs_node_child(m_segment->root(), "mehd");
        vhs_mehd_box_t *mehd = box ? VHS_MEHD_BOX(vhs_node_raw(box)) : NULL;
        if (mehd) {
            m_duration = vhs_mehd_duration(mehd);
        } else {
            printf("ERROR TODO: InitializationSegmentVHS::duration() MVHD duration!!!!!\n");
            // vhs_node_t *box = vhs_node_child(m_segment->root(), "mvhd");
            // vhs_mvhd_box_t *mvhd = box ? VHS_MVHD_BOX(vhs_node_raw(box)) : NULL;
            // if (mvhd) {
            //     m_duration = vhs_mvhd_timescale(mvhd);
            // }
        }
    }
    return m_duration;
}

uint32_t InitializationSegmentVHS::trackId() const {
    if (!m_trackId) {
        vhs_node_t *box = vhs_node_child(m_segment->root(), "tkhd");
        vhs_tkhd_box_t *tkhd = box ? VHS_TKHD_BOX(vhs_node_raw(box)) : NULL;
        if (tkhd) {
            m_trackId = vhs_tkhd_track_id(tkhd);
        } else {
            m_trackId = 1;
        }
    }
    printf("%s vhs_tkhd_track_id:%u\n", __PRETTY_FUNCTION__, m_trackId);
    return m_trackId;
}

// MediaSampleVHSDefaultsProxy overrides

// TODO: Cache these values
uint32_t InitializationSegmentVHS::mediaTimescale() const {
    uint32_t mediaTimescale = 0;
    vhs_node_t *box = vhs_node_child(m_segment->root(), "mdhd");
    vhs_mdhd_box_t *mdhd = box ? VHS_MDHD_BOX(vhs_node_raw(box)) : NULL;
    if (mdhd) {
        mediaTimescale = vhs_mdhd_timescale(mdhd);
    }
    return mediaTimescale;
}

uint64_t InitializationSegmentVHS::sampleDuration() const {
    uint64_t sampleDuration = 0;

    vhs_node_t *box = vhs_node_child(m_segment->root(), "tkhd");
    vhs_tkhd_box_t *tkhd = box ? VHS_TKHD_BOX(vhs_node_raw(box)) : NULL;
    if (tkhd && vhs_tkhd_duration_is_max(tkhd)) {
        memset(&sampleDuration, 1, sizeof(uint64_t));
    } else if (tkhd) {
        sampleDuration = vhs_tkhd_duration(tkhd);
    }
    // NOTE(estobbart): Some content omits the duration in the tkhd
    if (!sampleDuration) {
        vhs_node_t *box = vhs_node_child(m_segment->root(), "trex");
        vhs_trex_box_t *trex = box ? VHS_TREX_BOX(vhs_node_raw(box)) : NULL;
        if (trex) {
            vhs_sample_t *sampleDefaults = vhs_trex_default_sample_values(trex);
            sampleDuration = vhs_sample_duration(sampleDefaults);
        }
    }

    return sampleDuration;
}

FloatSize InitializationSegmentVHS::trackPresentationSize() const {
    float width = 0.0f;
    float height = 0.0f;

    vhs_node_t *box = vhs_node_child(m_segment->root(), "tkhd");
    vhs_tkhd_box_t *tkhd = box ? VHS_TKHD_BOX(vhs_node_raw(box)) : NULL;
    if (tkhd) {
        width = vhs_tkhd_width(tkhd);
        height = vhs_tkhd_height(tkhd);
        printf("%s %f %f\n", __PRETTY_FUNCTION__, width, height);
    }

    return FloatSize(width, height);
}

VHSSampleType InitializationSegmentVHS::sampleType() const {
    // printf("%s\n", __PRETTY_FUNCTION__);

    VHSSampleType result = VHSUnknownSampleType;

    vhs_node_t *box = vhs_node_child(m_segment->root(), "hdlr");
    vhs_hdlr_box_t *hdlr = box ? VHS_HDLR_BOX(vhs_node_raw(box)) : NULL;
    if (hdlr) {
        // printf("hdlr type: %s\n", vhs_hdlr_handler_type(hdlr));
        // VideoHandler / SoundHandler
        if (strcmp(vhs_hdlr_handler_type(hdlr), "vide") == 0) {
            result = VHSVideoSampleType;
        } else if (strcmp(vhs_hdlr_handler_type(hdlr), "soun") == 0) {
            result = VHSAudioSampleType;
        }
    }

    return result;
}

void * InitializationSegmentVHS::decoderConfig() const {

    if (m_audioConfig) {
        return m_audioConfig;
    }

    if (m_videoConfig) {
        return m_videoConfig;
    }

    return NULL;
}

}
