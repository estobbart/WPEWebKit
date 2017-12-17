//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "SourceBufferPrivateHelio.h"
#include "MediaSampleHelio.h"
#include "VideoTrackPrivateHelio.h"
#include "AudioTrackPrivateHelio.h"
#include "MediaSourcePrivateHelio.h"
#include "MediaPlayerPrivateHelio.h"
#include "avc.h"
#include "pes.h"
#include "aac.h"
//#include "InbandTextTrackPrivate.h"


#include <stdio.h>
#include <string.h> // memcpy

#import <wtf/MainThread.h>

namespace WebCore {

// static void _ftyp_box_callback(void *ctx, rcv_node_t *box) {
//     // TODO:
//     // 1. A File Type Box contains a major_brand or compatible_brand that the
//     // user agent does not support.
//     // printf("_ftyp_box_callback");
// }

// static void _frma_box_callback(void *ctx, rcv_node_t *box) {
//     // TODO:
//     // 2. A box or field in the Movie Box is encountered that violates the
//     // requirements mandated by the major_brand or one of the compatible_brands
//     // in the File Type Box.
//     // printf("_frma_box_callback");
//
//     /*
//
//     ftyp
//     mp41
//
//     stsd->mp4a
//
//
//     */
//     rcv_frma_box_t *frma = RCV_FRMA_BOX(rcv_node_raw(box));
//     char *format = rcv_frma_format(frma);
//
//     SourceBufferPrivateHelio *sbHelio = (SourceBufferPrivateHelio *)ctx;
//     sbHelio->setTrackDescription(MediaDescriptionHelio::create(format));
// }

// static void _track_box_callback(void *ctx, rcv_node_t *box) {
//     // TODO:
//     // 3. The tracks in the Movie Box contain samples (i.e., the entry_count in
//     // the stts, stsc or stco boxes are not set to zero).
//     // printf("_track_box_callback");
// }

// static void _mvex_box_callback(void *ctx, rcv_node_t *box) {
//     // TODO:
//     // 4. A Movie Extends (mvex) box is not contained in the Movie (moov) box to
//     // indicate that Movie Fragments are to be expected.
//     // printf("_mvex_box_callback");
// }
    
static void rcv_media_read_callback_source_buffer(void *context) {
    printf("SourceBufferPrivateHelio rcv_media_read_callback_source_buffer\n");
    SourceBufferPrivateHelio *sb = (SourceBufferPrivateHelio *)context;
    
    sb->platformBufferAvailable();
}


    RefPtr<SourceBufferPrivateHelio> SourceBufferPrivateHelio::create(MediaPlayerPrivateHelio *player,
                                                                      MediaSourcePrivateHelio *parent,
                                                                      String codec) {
    printf("SourceBufferPrivateHelio create\n");
    return adoptRef(new SourceBufferPrivateHelio(player, parent, codec));
}

SourceBufferPrivateHelio::SourceBufferPrivateHelio(MediaPlayerPrivateHelio *player,
                                                   MediaSourcePrivateHelio *parent,
                                                   String codec)
  : m_readyState(MediaPlayer::HaveNothing)
  , m_weakFactory(this)
  , m_mediaPlayer(player)
  , m_mediaSource(parent)
  , m_writeBufferAvailable(false)
  , m_isAudio(false)
  , m_isVideo(false) {
      printf("SourceBufferPrivateHelio constructor: %s\n", codec.utf8().data());

    m_isobmffInitSegmentRoot = NULL;
    m_enqueuedSample = NULL;
      
    // TODO: Create a "program" to play based on the content type and codec
    m_mediaPipeline = NULL;
    m_mediaStream = rcv_media_stream_init(rcv_media_read_callback_source_buffer,
                                          this,
                                          codec.utf8().data());
    m_rcvmfParser = rcv_parser_init(NULL, NULL);
}

SourceBufferPrivateHelio::~SourceBufferPrivateHelio() {
    printf("SourceBufferPrivateHelio destructor\n");
    if (m_rcvmfParser) {
        rcv_parser_destroy(&m_rcvmfParser);
    }
    if (m_mediaStream) {
        // TODO: Destory the stream
    }
    if (m_mediaPipeline) {
        // TODO: destory the pipeline
    }
}

void SourceBufferPrivateHelio::setClient(SourceBufferPrivateClient* client) {
    printf("SourceBufferPrivateHelio setClient\n");
    m_client = client;
}

/**
 * This method needs to report back to the client
 * https://rawgit.com/w3c/media-source/c3ad59c7a370d04430969ba73d18dc9bcde57a33/index.html#init-segment
 * Initialization Segment
 * A sequence of bytes that contain all of the initialization information
 * required to decode a sequence of media segments. This includes codec
 * initialization data, Track ID mappings for multiplexed segments, and
 * timestamp offsets (e.g. edit lists).
 *
 * https://www.w3.org/TR/mse-byte-stream-format-isobmff/#iso-media-segments
 *
 * We first must report an InitializationSegment prior to reporting
 * samples, or appendError gets called.
 * https://www.w3.org/TR/media-source/#sourcebuffer-init-segment-received
 * https://www.w3.org/TR/media-source/#init-segment
 *
 * A Segment has four attributes..
 *  it's duration,
 *  a vector of audioTrack information,
 *  a vector of videoTracks information,
 *  and a vector of textTrack information.
 *
 *  within those information vectors includes MediaDescription, and
 *  a trackPrivate ref.
 *
 * A MediaDescription includes an AtomicString of the codec for the track
 * and a bool to determine if the track is audio, video, or text.
 *
 * The *TrackPrivate ref, is somewhat dumb, and doesn't carry much
 * information with it.
 *
 *
 */
void SourceBufferPrivateHelio::append(const unsigned char* data,
                                      unsigned length) {
    printf("SourceBufferPrivateHelio append size:%i\n", length);

    bool appendSuccess = false;
    if (!m_client) {
        return;
    }

    rcv_node_t *root = rcv_node_init();
    rcv_parse_isobmff(m_rcvmfParser, root, data, length);

//    printf("SourceBufferPrivateHelio rcv_parse_isobmff(m_rcvmfParser, root, data, length) DONE;\n");

    if (rcv_node_child(root, "mdat")) {
        
        appendSuccess = this->didDetectISOBMFFMediaSegment(root);
        
        if (!appendSuccess) {
            rcv_destory_tree(&root);
        }

    } else if (rcv_node_child(root, "tkhd")) {
        // TODO: ^ this is sort a bad else if, moov & moof instead?

        appendSuccess = this->didDetectISOBMFFInitSegment(root);
        
        m_isobmffInitSegmentRoot = root;
        
//        rcv_destory_tree(&root);
    }
    
    printf("SourceBufferPrivateHelio sourceBufferPrivateAppendComplete %s\n",
           appendSuccess ? "AppendSucceeded" : "ERROR: ParsingFailed");
    
    m_client->sourceBufferPrivateAppendComplete(this,
        appendSuccess ? SourceBufferPrivateClient::AppendSucceeded : SourceBufferPrivateClient::ParsingFailed);

}

bool SourceBufferPrivateHelio::didDetectISOBMFFInitSegment(rcv_node_t *root) {
    bool reportedInitSegment = false;

    rcv_node_t *box;
    box = rcv_node_child(root, "tkhd");
    if (box) {
        
        SourceBufferPrivateClient::InitializationSegment segment;
        
        rcv_tkhd_box_t *tkhd = RCV_TKHD_BOX(rcv_node_raw(box));
        
        uint32_t id = rcv_tkhd_track_id(tkhd);
        
        box = rcv_node_child(root, "mvhd");
        if (box) {
            rcv_mvhd_box_t *mvhd = RCV_MVHD_BOX(rcv_node_raw(box));
            m_timescale = rcv_mvhd_timescale(mvhd);
            //            if (m_timescale == 0) {
            //                printf("MVHD DID NOT RETURN PROPER TIMESCALE USING 90000\n");
            //                m_timescale = 90000;
            //            }
            //        } else {
            //            printf("NO SUCH MVHD BOX MVHD TIMESCALE USING 90000\n");
            //            m_timescale = 90000;
        }
        
        // NOTE: We're going to stick to the TKHD since it's a track we work
        // with at this layer..
        if (rcv_tkhd_duration_is_max(tkhd)) {
            segment.duration = MediaTime::positiveInfiniteTime();
        } else {
            // TODO: Warn that we may lose precision here..
            // MediaTime(int64_t value, int32_t scale, uint32_t flags = Valid);
            // m_timescale is a uint32_t
            uint64_t duration = rcv_tkhd_duration(tkhd);
            segment.duration = MediaTime(duration, m_timescale);
        }

        // TODO: Change this to... sample->type == avc1
        //                         sample->type == mp4a
        box = rcv_node_child(root, "stsd");
        if (box) {
            printf("SourceBufferPrivateHelio Found STSD box\n");
        } else {
            printf("SourceBufferPrivateHelio ERROR FINDING STSD box\n");
        }

//        if (rcv_stsd_sample_entry_type(RCV_STSD_BOX(box))) {
//            m_trackDescription = MediaDescriptionHelio::create(rcv_stsd_sample_entry_type(RCV_STSD_BOX(box)));
//        } else {
        
//            printf("SourceBufferPrivateHelio:: rcv_stsd_sample_entry_type%s\n",
//                   rcv_stsd_sample_entry_type(RCV_STSD_BOX(box)));
            box = rcv_node_first_child(box);

            char *s = rcv_node_type(box);

            m_trackDescription = MediaDescriptionHelio::create(strcmp(s, "avcC") == 0 ? "avc1" : "mp4a");
//        }

        rcv_node_t *child_box = rcv_node_child(root, "hdlr");
        if (child_box) {
            rcv_hdlr_box_t *hdlr = RCV_HDLR_BOX(rcv_node_raw(child_box));

            // TODO: Audio samples.. Video Samples flags..
            char *format = rcv_hdlr_handler_type(hdlr);

            printf("SourceBufferPrivateHelio rcv_stsd_sample_entry_type... %s\n", format);

            if (strcmp(format, "vide") == 0) {
                rcv_nal_param_t **sps = (rcv_nal_param_t **)malloc(sizeof(rcv_nal_param_t *) * 2);
                sps[0] = malloc(sizeof(rcv_nal_param_t));
//                sps[0]->nal_unit = sps_nal;
//                sps[0]->nal_len = sps_len;
                sps[1] = malloc(sizeof(rcv_nal_param_t));
                sps[1]->nal_unit = NULL;
                sps[1]->nal_len = 0;
                //printf("SourceBufferPrivateHelio rcv_nal_param_t **sps\n");

                rcv_nal_param_t **pps = (rcv_nal_param_t **)malloc(sizeof(rcv_nal_param_t *) * 2);
                pps[0] = malloc(sizeof(rcv_nal_param_t));
//                pps[0]->nal_unit = pps_nal;
//                pps[0]->nal_len = pps_len;
                pps[1] = malloc(sizeof(rcv_nal_param_t));
                pps[1]->nal_unit = NULL;
                pps[1]->nal_len = 0;
                //printf("SourceBufferPrivateHelio rcv_nal_param_t **pps\n");


                rcv_node_t *avcC_node = rcv_node_child(root, "avcC");
                rcv_avcC_box_t *avcC = RCV_AVCC_BOX(rcv_node_raw(avcC_node));

                rcv_cursor_t *sps_cursor = rcv_cursor_init();
                rcv_avcC_parameter_set_t *param_set = rcv_avcC_sps_next(avcC, &sps_cursor);
                rcv_avcC_parameter_set_assign_and_free(&param_set, &sps[0]->nal_unit, &sps[0]->nal_len);
                if (sps_cursor != NULL) {
                    // TODO: This might mean we actually have to call next a second time
                    // this may be a bug in the cursor code
                    printf("SourceBufferPrivateHelio ERROR sps_cursor not freed\n");
                }
                if (param_set != NULL) {
                    printf("SourceBufferPrivateHelio ERROR param_set not freed\n");
                }

                rcv_cursor_t *pps_cursor = rcv_cursor_init();
                param_set = rcv_avcC_pps_next(avcC, &pps_cursor);
                rcv_avcC_parameter_set_assign_and_free(&param_set, &pps[0]->nal_unit, &pps[0]->nal_len);
                if (pps_cursor != NULL) {
                    // TODO: See above.
                    printf("SourceBufferPrivateHelio ERROR sps_cursor not freed\n");
                }
                if (param_set != NULL) {
                    printf("SourceBufferPrivateHelio ERROR param_set not freed\n");
                }

                rcv_param_sets_t *param_sets = malloc(sizeof(rcv_param_sets_t));
                param_sets->sps = sps;
                param_sets->pps = pps;
                m_codecConfiguration = HelioVideoCodecConfiguration::create(param_sets);

                m_isVideo = true;
                SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
                vtInfo.track = VideoTrackPrivateHelio::create(id);
                vtInfo.description = m_trackDescription;
                printf("SourceBufferPrivateHelio VideoTrackPrivateHelio m_id:%s\n", vtInfo.track->id().string().utf8().data());

                segment.videoTracks.append(vtInfo);
            } else if (strcmp(format, "soun") == 0) {
                // TODO: Defaults for now.. this really should put the buffer
                // into an error state, but we need to check the library has
                // no bugs.
                uint8_t aot = 2;
                uint8_t sfi = 6;
                uint8_t cc = 2;
                rcv_node_t *esds_node = rcv_node_child(root, "esds");
                if (esds_node) {
                    rcv_esds_box_t *esds = RCV_ESDS_BOX(rcv_node_raw(esds_node));
                    if (esds) {
                        rcv_decoder_specific_info_t *dsi = rcv_esds_decoder_specifc_info(esds);
                        if (dsi) {
                            aot = rcv_esds_dsi_audio_object_type(dsi);
                            sfi = rcv_esds_dsi_sampling_frequency_index(dsi);
                            cc = rcv_esds_dsi_channel_config(dsi);
                        } else {
                            printf("SourceBufferPrivateHelio ERROR: NO DSI\n");
                        }
                    } else {
                        printf("SourceBufferPrivateHelio ERROR: NO ESDS\n");
                    }
                } else {
                    printf("SourceBufferPrivateHelio ERROR: NO ESDS NODE\n");
                }

                m_codecConfiguration = HelioAudioCodecConfiguration::create(aot, sfi, cc);

                m_isAudio = true;
                SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
                atInfo.track = AudioTrackPrivateHelio::create(id);
                atInfo.description = m_trackDescription;

                printf("SourceBufferPrivateHelio AudioTrackPrivateHelio m_id:%s\n", atInfo.track->id().string().utf8().data());
                segment.audioTracks.append(atInfo);
            }

            printf("SourceBufferPrivateHelio sourceBufferPrivateDidReceiveInitializationSegment\n");
            m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);
            reportedInitSegment = true;

        }
    }

    return reportedInitSegment;

}

bool SourceBufferPrivateHelio::didDetectISOBMFFMediaSegment(rcv_node_t *root) {

    // TODO: Does this local var produce ref count churn?
    Ref<MediaSample> mediaSample = MediaSampleHelio::create(root,
                                                            m_timescale,
                                                            m_codecConfiguration);

    printf("SourceBufferPrivateHelio sourceBufferPrivateDidReceiveSample\n");
    m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);
    
    return true;
}

void SourceBufferPrivateHelio::abort() {
    printf("SourceBufferPrivateHelio abort\n");
}

void SourceBufferPrivateHelio::resetParserState() {
    printf("SourceBufferPrivateHelio resetParserState\n");
}

void SourceBufferPrivateHelio::removedFromMediaSource() {
    printf("SourceBufferPrivateHelio removedFromMediaSource\n");
}

MediaPlayer::ReadyState SourceBufferPrivateHelio::readyState() const {
    printf("SourceBufferPrivateHelio readyState\n");
    return m_readyState;
}

void SourceBufferPrivateHelio::setReadyState(MediaPlayer::ReadyState readyState) {
    printf("SourceBufferPrivateHelio setReadyState:%i\n", readyState);
    m_readyState = readyState;
}

void SourceBufferPrivateHelio::flush(AtomicString as) {
    printf("TODO: SourceBufferPrivateHelio flush:%s\n", as.string().utf8().data());
}
    
void hex_dump_buffer(uint8_t *buffer, size_t size) {
    uint8_t count = 0;
    while (size) {
        printf(" %02x", buffer[0]);
        size--;
        buffer++;
        count++;
        if (count == 32) {
            printf("\n");
            count = 0;
        }
    }
    if (count) {
        printf("\n");
    }
}
    
void SourceBufferPrivateHelio::enqueueSample(PassRefPtr<MediaSample> mediaSample, AtomicString as) {
    printf("SourceBufferPrivateHelio enqueueSample:%s\n", as.string().utf8().data());
    m_writeBufferAvailable = false;

    //uint8_t *mdatBuffer;
    //size_t size;

    // Pointer to the buffer we can fill with PES packets
    uint8_t *writeBuffer = NULL;
    // number of bytes available in the buffer we can fill
    size_t totalAvailable = 0;
    size_t bufferAvailable = 0;
    // rcv_media_stream_write(m_mediaStream, writeBuffer, writeSize);
    if (!rcv_media_stream_buffer(m_mediaStream, &writeBuffer, &totalAvailable)) {
        printf("ERROR SourceBufferPrivateHelio unable to get buffer from pool\n");
        m_enqueuedSample = static_cast<MediaSampleHelio *>(mediaSample.get());
        return;
    }
    
//    while (totalAvailable < 50000) {
//        printf("ERROR SourceBufferPrivateHelio totalAvailable < 500000\n sleep...");
//        WTF::sleep(2);
//        rcv_media_stream_buffer(m_mediaStream, &writeBuffer, &totalAvailable);
//        printf("SourceBufferPrivateHelio NEXUS buffer available:%zu\n", totalAvailable);
//    }
    
    bufferAvailable = totalAvailable;

    printf("SourceBufferPrivateHelio static_cast<MediaSampleHelio *>\n");
    MediaSampleHelio* helioSample = static_cast<MediaSampleHelio *>(mediaSample.get());
    printf("SourceBufferPrivateHelio NEXUS buffer available:%zu\n", bufferAvailable);
    while (bufferAvailable >= helioSample->sizeOfNextPESPacket()) {
        size_t writeSize = 0;
        if (!helioSample->writeNextPESPacket(&writeBuffer, &writeSize)) {
            printf("SourceBufferPrivateHelio::enqueueSample writeNextPESPacket BREAK\n");
            break;
        }
        bufferAvailable -= writeSize;
        writeBuffer += writeSize;
//        printf("SourceBufferPrivateHelio::enqueueSample writeNextPESPacket:%zu\n", writeSize);
    }
    
    // We've got more samples to write, but probably ran out of space.
    if (helioSample->sizeOfNextPESPacket()) {
        printf("SourceBufferPrivateHelio::enqueueSample ERROR Data to write in this sample, but no more space in the buffer, m_enqueuedSample%zu\n", helioSample->sizeOfNextPESPacket());
        m_enqueuedSample = helioSample;
    }


//    printf("SourceBufferPrivateHelio helioSample->sampleBuffer DONE\n");
//
//    rcv_trun_box_t *trun = RCV_TRUN_BOX(helioSample->box("trun"));
//    printf("SourceBufferPrivateHelio RCV_TRUN_BOX(helioSample->box('trun'))\n");
//
////    if (!m_enqueuedSampleCursor) {
////        m_enqueuedSampleCursor = rcv_cursor_init();
////    }
//
//    rcv_tfdt_box_t *tfdt = RCV_TFDT_BOX(helioSample->box("tfdt"));
//    uint64_t decodeTimeBase = rcv_tfdt_base_media_decode_time(tfdt);
//    //printf("decodeTimeBase:%llu\n", decodeTimeBase);
//
//    // Base 187680 + CTO (-3754) + duration (3754 x 2) = 191434
//    uint64_t durationSum = 0;
//    
//    uint32_t sampleSize = 0;
//    
//    rcv_sample_t *sample;
//
//    if (m_isVideo) {
//        printf("SourceBufferPrivateHelio isVideo\n");
//        //typedef uint32_t (*nalu_sample_size_next)(void *ctx);
//
//        //typedef bool (*nalu_key_frame_headers_next)(void *ctx, uint8_t **nal_unit, uint16_t *length);
//
//        // TODO: Might be easier to just pass the avcC config here..
//
////        rcv_nal_param_t **sps = (rcv_nal_param_t **)malloc(sizeof(rcv_nal_param_t *) * 2);
////        sps[0] = malloc(sizeof(rcv_nal_param_t));
////        sps[0]->nal_unit = sps_nal;
////        sps[0]->nal_len = sps_len;
////        sps[1] = malloc(sizeof(rcv_nal_param_t));
////        sps[1]->nal_unit = NULL;
////        sps[1]->nal_len = 0;
//        //printf("SourceBufferPrivateHelio rcv_nal_param_t **sps\n");
//
////        rcv_nal_param_t **pps = (rcv_nal_param_t **)malloc(sizeof(rcv_nal_param_t *) * 2);
////        pps[0] = malloc(sizeof(rcv_nal_param_t));
////        pps[0]->nal_unit = pps_nal;
////        pps[0]->nal_len = pps_len;
////        pps[1] = malloc(sizeof(rcv_nal_param_t));
////        pps[1]->nal_unit = NULL;
////        pps[1]->nal_len = 0;
//        //printf("SourceBufferPrivateHelio rcv_nal_param_t **pps\n");
//
////        rcv_param_sets_t param_sets = { sps, pps };
//
//        // This is a mess.. need a better way to get this size..
//        // Create a write buffer the size of the mdatBuffer + pps + sps + PES
//        // printf("SourceBufferPrivateHelio size:%zu sps_len:%u pps_len:%u", size, sps_len, pps_len);
//        // writeSize = size + sps_len + 4 + pps_len + 4;
//        // * 14 needs to happen PER sample.. right now it's only a single sample.
//        // printf("SourceBufferPrivateHelio sample count:%u\n", rcv_trun_sample_count(trun));
//        // writeSize += rcv_trun_sample_count(trun) * 14;
//        // printf("SourceBufferPrivateHelio writeSize %zu\n", writeSize);
//        // writeBuffer = malloc(writeSize);
//
//        sample = rcv_trun_sample_next(trun, &m_enqueuedSampleCursor);
//        //printf("cto_apply:%llu\n", (durationSum + decodeTimeBase));
//        //printf("duration:%lu size:%lu cto:%llu\n", rcv_trun_sample_duration(sample), rcv_trun_sample_size(sample), rcv_trun_sample_apply_cto(trun, sample, durationSum + decodeTimeBase));
//
//        cvmf_pes_t *pes = cvmf_pes_create(cvmf_pes_type_video);
//        // This comes in as 32 bits, but the size can only be 16 bits..
//        // TODO: Add an additional check here
//        sampleSize = rcv_trun_sample_size(sample);
//        printf("SourceBufferPrivateHelio sampleSize:%lu\n", sampleSize);
//        cvmf_pes_set_size(pes, sampleSize + sps_len + 4 + pps_len + 4);
//
//        // even though we allow a uint64_t here the max value is
//        // 8589934591 (0x1FFFFFFFF) 33bits.
//        //printf("1 decodeTimeBase:%llu rcv_trun_sample_cto(sample):%lu durationSum:%llu\n", decodeTimeBase, rcv_trun_sample_cto(sample), durationSum);
//        //printf("1 decodeTimeBase:%llu\n", decodeTimeBase);
//        cvmf_pes_set_pts(pes, rcv_trun_sample_apply_cto(trun, sample, durationSum + decodeTimeBase));
//        durationSum += rcv_trun_sample_duration(sample);
//
//        // TODO: Write PES..
//        uint8_t *pesBuffer = NULL;
//        size_t pesSize = 0;
//        cvmf_pes_raw(pes, &pesBuffer, &pesSize);
//        // void *memcpy(void *dest, const void *src, size_t n);
//        // TODO: Need a write offset..
//        if (bufferAvailable >= pesSize) {
//            memcpy(writeBuffer, pesBuffer, pesSize);
//            printf("SourceBufferPrivateHelio writeBuffer:%p\n", writeBuffer);
//            writeBuffer += pesSize;
//            bufferAvailable -= pesSize;
//        }
//        
//        
//        //uint8_t *writePointer = &writeBuffer[pesSize];
//
//        // TODO: Write ES sample
//        avc_frame_to_annex_b(&param_sets, 3,
//                             mdatBuffer, sampleSize,
//                             [](void *ctx, uint8_t *buffer, size_t size){
//                                 uint8_t **wb = (uint8_t **)ctx;
//                                 //printf("writePointer:%p\n", *wb);
//                                 memcpy(*wb, buffer, size);
//                                 *wb += size;
//                             }, &writeBuffer);
//        mdatBuffer += sampleSize;
//        bufferAvailable -= sampleSize + sps_len + 4 + pps_len + 4;
//
//        //printf("2 decodeTimeBase:%llu\n", decodeTimeBase);
//
//        while (sample != NULL) {
//            sample = rcv_trun_sample_next(trun, &m_enqueuedSampleCursor);
//            if (sample == NULL) {
//                break;
//            }
//
//            // printf("duration:%lu size:%lu cto:%llu\n", rcv_trun_sample_duration(sample), rcv_trun_sample_size(sample), rcv_trun_sample_apply_cto(trun, sample, durationSum + decodeTimeBase));
//
//            // This comes in as 32 bits, but the size can only be 16 bits..
//            // TODO: Add an additional check here
//            sampleSize = rcv_trun_sample_size(sample);
//            
//            if (sampleSize + 14 > bufferAvailable) {
//                printf("WARN SourceBufferPrivateHelio out of space in buffer\n");
//                m_enqueuedSample = static_cast<MediaSampleHelio *>(mediaSample.get());
//                break;
//            }
//
//            // printf("sampleSize:%lu\n", sampleSize);
//            cvmf_pes_set_size(pes, sampleSize);
//
//            // even though we allow a uint64_t here the max value is
//            // 8589934591 (0x1FFFFFFFF) 33bits.
//            cvmf_pes_set_pts(pes, rcv_trun_sample_apply_cto(trun, sample, durationSum + decodeTimeBase));
//            durationSum += rcv_trun_sample_duration(sample);
//
//            // TODO: Write PES..
//            // cvmf_pes_raw(cvmf_pes_t *pes, uint8_t **raw, size_t *size);
//            cvmf_pes_raw(pes, &pesBuffer, &pesSize);
//            memcpy(writeBuffer, pesBuffer, pesSize);
//            writeBuffer += pesSize;
//            bufferAvailable -= pesSize;
//
//            // TODO: Write ES sample
//            // TODO: This needs the same pointer
//            avc_frame_to_annex_b(NULL, 3,
//                                 mdatBuffer, sampleSize,
//                                 [](void *ctx, uint8_t *buffer, size_t size){
//                                     uint8_t **wb = (uint8_t **)ctx;
//                                     //printf("writePointer:%p\n", *wb);
//                                     memcpy(*wb, buffer, size);
//                                     *wb += size;
//                                 }, &writeBuffer);
//            mdatBuffer += sampleSize;
//            bufferAvailable -= sampleSize;
//        }
//
//        cvmf_pes_destroy(&pes);
//
//
////        printf("SourceBufferPrivateHelio avc_to_annex_b\n");
////        avc_to_annex_b(&param_sets, sample_size_iterator, &sample_ctx,
////                       rcv_trun_sample_count(trun), 3,
////                       &mdatBuffer, &size);
//        printf("SourceBufferPrivateHelio avc_to_annex_b DONE\n");
//        free(sps[0]);
//        free(sps[1]);
//        free(sps);
//        free(pps[0]);
//        free(pps[1]);
//        free(pps);
//        printf("SourceBufferPrivateHelio FREEING... DONE\n");
//        //first_sample_size = sample_ctx.one_sample;
//    } else {
//        // AUDIO...
//        
////        printf("SourceBufferPrivateHelio sample count:%u\n", rcv_trun_sample_count(trun));
////        writeSize = size + (rcv_trun_sample_count(trun) * 21);
////        printf("SourceBufferPrivateHelio writeSize %zu\n", writeSize);
////        writeBuffer = malloc(writeSize);
////        
////        uint8_t *writePointer = writeBuffer;
//        uint8_t *pesBuffer = NULL;
//        size_t pesSize = 0;
//        cvmf_pes_t *pes = cvmf_pes_create(cvmf_pes_type_audio);
//
//        
//        // printf("SourceBufferPrivateHelio esds..\n");
//        rcv_esds_box_t *esds = RCV_ESDS_BOX(helioSample->box("esds"));
//        // printf("SourceBufferPrivateHelio rcv_esds_decoder_specifc_info\n");
//        rcv_decoder_specific_info_t *dsi = rcv_esds_decoder_specifc_info(esds);
//        // printf("SourceBufferPrivateHelio rcv_esds_decoder_specifc_info audio_type, sample, channel\n");
//        // TODO: Fix this!!!!!!!!!!!!!!!! the data is in the initSegment
//        uint8_t aot = 2;//rcv_esds_dsi_audio_object_type(dsi);
//        uint8_t sfi = 6;//rcv_esds_dsi_sampling_frequency_index(dsi);
//        uint8_t cc = 2;//rcv_esds_dsi_channel_config(dsi);
//        // This comes in as 32 bits, but the size can only be 16 bits..
//        // TODO: Add an additional check here
//        do {
//            sample = rcv_trun_sample_next(trun, &m_enqueuedSampleCursor);
//            if (sample == NULL) {
//                break;
//            }
//            sampleSize = rcv_trun_sample_size(sample);
//            
//            if (sampleSize + 14 + 7 > bufferAvailable) {
//                printf("WARN SourceBufferPrivateHelio out of space in buffer\n");
//                m_enqueuedSample = static_cast<MediaSampleHelio *>(mediaSample.get());
//                break;
//            }
//
//            // plus the ADTS size (7)
//            //printf("set_size:%lu\n", sampleSize + 7);
//            cvmf_pes_set_size(pes, sampleSize + 7);
//            
//            cvmf_pes_set_pts(pes, rcv_trun_sample_apply_cto(trun, sample, durationSum + decodeTimeBase));
//            
//            durationSum += rcv_trun_sample_duration(sample);
//            
//            cvmf_pes_raw(pes, &pesBuffer, &pesSize);
//            memcpy(writeBuffer, pesBuffer, pesSize);
////            printf("writeBuffer:%p\n", writePointer);
//            writeBuffer += pesSize;
//            bufferAvailable -= pesSize;
//            
//            uint8_t *adtsbuffer = aac_adts_header_init(aot,
//                                                       sfi,
//                                                       cc,
//                                                       sampleSize);
//            memcpy(writeBuffer, adtsbuffer, 7);
//            //            printf("writeBuffer:%p\n", writePointer);
//            writeBuffer += 7;
//            bufferAvailable -= 7;
//            memcpy(writeBuffer, mdatBuffer, sampleSize);
//            mdatBuffer += sampleSize;
//            writeBuffer += sampleSize;
//            bufferAvailable -= sampleSize;
//            
//        } while(sample);
//        
//        cvmf_pes_destroy(&pes);
//    }

    //printf("SourceBufferPrivateHelio WILL rcv_media_stream_write\n");
    
    // DEBUG
//    if (m_isVideo) {
//        hex_dump_buffer(buffer, first_sample_size + pps_len + sps_len + 4);
//    }

    if (writeBuffer == NULL) {
        return;
    }

    //printf("SourceBufferPrivateHelio will write.. writeBuffer:%p\n", writeBuffer);
    
    if (totalAvailable == bufferAvailable) {
        printf("SourceBufferPrivateHelio ERROR NO DATA WRITTEN TO THE BUFFER\n");
    }

    if (rcv_media_stream_write(m_mediaStream, totalAvailable - bufferAvailable)) {
        printf("SourceBufferPrivateHelio WRITE SUCCESS\n");
    } else {
        printf("ERROR: SourceBufferPrivateHelio WRITE ERROR\n");
    }
}

// SourceBuffer calls this method twice before calling the notify method.
// Once as soon as the sample is appended, and then a second time after calling
// append done. Make sure the event is fired before returning true here
// We don't want to block the event from dispatching.
bool SourceBufferPrivateHelio::isReadyForMoreSamples(AtomicString as) {
    printf("SourceBufferPrivateHelio isReadyForMoreSamples TRACK:%s %i\n",
           as.string().utf8().data(), m_writeBufferAvailable);

    return m_writeBufferAvailable;
}

void SourceBufferPrivateHelio::setActive(bool active) {
    // At this point we should assume we're going to need an a/v pipeline
    // for this content.
    printf("SourceBufferPrivateHelio setActive:%i\n", active);
    
    if (active && m_mediaStream) {
        printf("SourceBufferPrivateHelio creating pipeline\n");
        m_mediaPipeline = rcv_media_pipeline_init(m_mediaStream,
                                                  m_mediaPlayer->platformClockController());
    }
    // TODO: else rcv_media_pipeline_destroy??

    // TODO: Why does this need the m_mediaPipeline??
    m_mediaSource->sourceBufferPrivateActiveStateChanged(this, m_mediaPipeline);

}

void SourceBufferPrivateHelio::notifyClientWhenReadyForMoreSamples(AtomicString trackId) {
    // TODO:
    printf("ERROR: SourceBufferPrivateHelio notifyClientWhenReadyForMoreSamples:%s ignoring\n", trackId.string().utf8().data());
}
    
//void SourceBufferPrivateHelio::writeBufferAvailable() {
//    //m_readyForMoreSamples = true;
//}
    
// Called by the MediaSource as soon as all it's buffers become active
// and the pipelines are connected.
void SourceBufferPrivateHelio::startStream() {
    printf("SourceBufferPrivateHelio::startStream()\n");
    // TODO: at this point we MAY be ready for samples, but we wait until
    // the stream's data ready callback occurs.
    rcv_media_pipeline_start_stream(m_mediaPipeline, m_mediaStream);
}

// callback from rcvmf
void SourceBufferPrivateHelio::platformBufferAvailable() {
    printf("SourceBufferPrivateHelio::platformBufferAvailable()\n");
    if (m_enqueuedSample != NULL) {
        PassRefPtr<MediaSample> mediaSample = m_enqueuedSample;
        m_enqueuedSample = NULL;
        // TODO: Need the atomic string..
        printf("SourceBufferPrivateHelio::platformBufferAvailable(), enqueuing previous sample\n");
        enqueueSample(mediaSample, AtomicString("1"));
        return;
    }
    m_writeBufferAvailable = true;
}

}
