#import "config.h"
#import "MediaSampleHelio.h"

#import <wtf/PrintStream.h>
#import <math.h>

namespace WebCore {
    

uint32_t MediaSampleHelio::sizeOfNextPESPacket() {
    rcv_node_t *box = rcv_node_child(m_sample, "trun");
    rcv_trun_box_t *trun = box ? RCV_TRUN_BOX(rcv_node_raw(box)) : NULL;
    if (trun) {
        rcv_sample_t *sample = rcv_trun_sample_peek(trun, &m_cursor);
        if (sample) {
            // 14 byte PES header
            uint32_t pesSize = 14;
            pesSize += rcv_trun_sample_size(sample);
            if (m_hasAudio) {
                // 7 byte ADTS header,
                pesSize += 7;
            } else if (m_hasVideo && rcv_trun_sample_depends_on(sample) == rcv_sample_depends_false) {
                // printf("rcv_sample_depends_false\n");

                rcv_param_sets_t *paramSets = (rcv_param_sets_t *)m_codecConf->decoderConfiguartion();
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
    

bool MediaSampleHelio::writeNextPESPacket(uint8_t **buffer, size_t *size) {
    uint8_t *bufferHead = *buffer;
    rcv_node_t *box = rcv_node_child(m_sample, "trun");
    rcv_trun_box_t *trun = box ? RCV_TRUN_BOX(rcv_node_raw(box)) : NULL;
    
    uint8_t *mdatBuffer = NULL;
    size_t mdatSize = 0;
    sampleBuffer(&mdatBuffer, &mdatSize);
    
    
    box = rcv_node_child(m_sample, "tfdt");
    rcv_tfdt_box_t *tfdt = box ? RCV_TFDT_BOX(rcv_node_raw(box)) : NULL;

    if (trun && mdatBuffer && tfdt) {
        rcv_sample_t *sample = rcv_trun_sample_next(trun, &m_cursor);
        if (sample) {
            uint64_t pts = (m_ptsAccumulation
              + rcv_tfdt_base_media_decode_time(tfdt)
              + round(m_timestampOffset.toFloat() / m_timescale));
            // TODO: Need to adjust by this timestamp
//            printf("TODO:: MediaSampleHelio::writeNextPESPacket m_timestampOffset.toFloat() == %f Current PTS == %lld\n", m_timestampOffset.toFloat(), pts);
            uint32_t sampleSize = rcv_trun_sample_size(sample);
            uint32_t headerSize = 0;
            if (m_hasAudio) {
                // 7 byte ADTS header,
                headerSize += 7;
            } else if (m_hasVideo) {
                // TODO: Move this condition into the else if
                if (rcv_trun_sample_depends_on(sample) == rcv_sample_depends_false) {
                    
                    // We don't consider this part of the header.
                    // the header would be if the nal_length didn't match
                    // the start_code 4 bytes, which is a a TODO to support
                    rcv_param_sets_t *paramSets = (rcv_param_sets_t *)m_codecConf->decoderConfiguartion();
                    sampleSize += paramSets->sps[0]->nal_len + 4;
                    sampleSize += paramSets->pps[0]->nal_len + 4;
                    
                }
                
                // TODO: We don't support nal_length != 4
                // Video only has an additional size if the nal_length
                // isn't 4 bytes & it's not an IFrame.
                
            } else {
                printf("ERROR: MediaSampleHelio::writeNextPESPacket !m_hasAudio && !m_hasVideo\n");
                return false;
            }

            cvmf_pes_set_size(m_pesPacket, sampleSize + headerSize);
            cvmf_pes_set_pts(m_pesPacket, pts);

            uint8_t *pesBuffer = NULL;
            size_t pesSize = 0;
            cvmf_pes_raw(m_pesPacket, &pesBuffer, &pesSize);

            memcpy(bufferHead, pesBuffer, pesSize);
            bufferHead += pesSize;
            *size += pesSize;
            
            // This sampleSize value is confusing.. we use it to
            // write the ESStream packet size, then reassign it to be able to
            // do the MDAT memcopy & move the readOffset.
            sampleSize = rcv_trun_sample_size(sample);
            m_ptsAccumulation += rcv_trun_sample_duration(sample);
            
            if (m_hasAudio) {
                rcv_aac_config_t *aacConf = (rcv_aac_config_t *)m_codecConf->decoderConfiguartion();
                aac_adts_header_write(aacConf, sampleSize, &bufferHead);
                bufferHead += headerSize;
                *size += headerSize;
                
                memcpy(bufferHead, &mdatBuffer[m_mdatReadOffset], sampleSize);
                *size += sampleSize;
                m_mdatReadOffset += sampleSize;
                
                return true;

            } else if (m_hasVideo) {
                rcv_param_sets_t *paramSets = NULL;
                if (rcv_trun_sample_depends_on(sample) == rcv_sample_depends_false) {
                  paramSets = (rcv_param_sets_t *)m_codecConf->decoderConfiguartion();
                }
                avc_write_frame_to_annex_b(paramSets,
                                           3, // TODO: hardcoded length - 1
                                           &mdatBuffer[m_mdatReadOffset],
                                           sampleSize,
                                           &bufferHead);
                if (paramSets) {
                    *size += paramSets->sps[0]->nal_len + 4;
                    *size += paramSets->pps[0]->nal_len + 4;
                }
                *size += sampleSize;
                m_mdatReadOffset += sampleSize;
                
                return true;
                
            }
        }
    } else {
        printf("ERROR: MediaSampleHelio::writeNextPESPacket !trun || !mdatBuffer || !tfdt\n");
    }
    return false;
    
}

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
    return m_presentationTime + m_timestampOffset;
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
    return m_decodeTime + m_timestampOffset;
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
    printf("MediaSampleHelio::sizeInBytes ");
    rcv_node_t *node = rcv_node_child(m_sample, "mdat");
    if (node) {
        rcv_mdat_box_t *mdat = RCV_MDAT_BOX(rcv_node_raw(node));
        size_t size = rcv_mdat_data_size(mdat);
        printf("%zu \n", size);
        return size;
    }
    printf("0 \n");
    return 0;
}

FloatSize MediaSampleHelio::presentationSize() const {
    printf("TODO: MediaSampleHelio::presentationSize\n");
    return FloatSize();
}

void MediaSampleHelio::offsetTimestampsBy(const MediaTime &mediaTime) {
    printf("MediaSampleHelio::offsetTimestampsBy %f\n", mediaTime.toFloat());
    m_timestampOffset = mediaTime;
}

// TODO: It's not clear when/why this would be called.
void MediaSampleHelio::setTimestamps(const MediaTime &mediaTimeOne __attribute__((unused)),
                                     const MediaTime &mediaTimeTwo __attribute__((unused))) {
  printf("TODO: MediaSampleHelio::setTimestamps\n");
}

// NOTE(estobb200): For video a sample would only be divisible if it had multiple
// keyframes in it that could be used as RAPs. Since in S8 the keyframes are
// only at the start of a sample it's not worth dividing them. We do need to
// support an iframe track, and in that case would get multiple keyframes in
// a sample. But it still TDB if it's worth dividing them.
// TODO: For audio we can divide while the sample count is > 1.
bool MediaSampleHelio::isDivisable() const {
    printf("MediaSampleHelio::isDivisable\n");
    return false;
}

std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> MediaSampleHelio::divide(const MediaTime& presentationTime __attribute__((unused))) {
    printf("TODO: MediaSampleHelio::divide\n");
    return { nullptr, nullptr };
}

// TODO: Create a non-display copy by freeing the MDAT box
Ref<MediaSample> MediaSampleHelio::createNonDisplayingCopy() const {
    printf("TODO: MediaSampleHelio::createNonDisplayingCopy\n");
    /**
     error: could not convert '(const WebCore::MediaSampleHelio*)this' from 'const WebCore::MediaSampleHelio*' to 'WTF::Ref<WebCore::MediaSample>'
     |      return this; //MediaSampleHelio::create(NULL, 0, m_codecConf);
     */
    return adoptRef(*this); //MediaSampleHelio::create(NULL, 0, m_codecConf);
}

// NOTE(estobb200): Every sample from super8 is considered a sync sample,
// See comments about being divisible.
MediaSample::SampleFlags MediaSampleHelio::flags() const {
    // TODO: What does audio look like here?
    printf("MediaSampleHelio::flags\n");
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
PlatformSample MediaSampleHelio::platformSample() {
    printf("MediaSampleHelio::platformSample\n");
    return PlatformSample();
}

void MediaSampleHelio::dump(PrintStream &ps __attribute__((unused))) const {
    // out.print("{PTS(", presentationTime(), "), DTS(", decodeTime(), "), duration(", duration(), "), flags(", (int)flags(), "), presentationSize(", presentationSize().width(), "x", presentationSize().height(), ")}");
    printf("MediaSampleHelio::dump...\n");
}
    
void MediaSampleHelio::sampleBuffer(uint8_t **buffer, size_t *size) {
    rcv_node_t *node = rcv_node_child(m_sample, "mdat");
    if (!node) {
        printf("ERROR: MediaSampleHelio::sampleBuffer NULL, 0 \n", *size);
        *buffer = NULL;
        *size = 0;
        return;
    }
    rcv_mdat_box_t *mdat = RCV_MDAT_BOX(rcv_node_raw(node));
    *buffer = rcv_mdat_data(mdat);
    *size = rcv_mdat_data_size(mdat);
    if (m_mdatReadOffset == 0) {
        printf("MediaSampleHelio::sampleBuffer ----> :%zu \n", *size);
    }
}
    
//void * MediaSampleHelio::box(char * fourcc) {
//    rcv_node_t *node = rcv_node_child(m_sample, fourcc);
//    if (node) {
//        return rcv_node_raw(node);
//    }
//    return NULL;
//}

}
