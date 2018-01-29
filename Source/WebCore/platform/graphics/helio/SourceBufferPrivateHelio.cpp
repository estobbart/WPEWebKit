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
#include "now.h"
//#include "InbandTextTrackPrivate.h"


#include <stdio.h>
#include <string.h> // memcpy

#include <sys/time.h> // gettimeofday

#import <wtf/MainThread.h>

//#import <pthread.h>
#include <thread>

#define SBPH_PRINT 0

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

static void rcv_pssh_box_callback(void *ctx, rcv_node_t *node) {
    printf("PSSH rcv_pssh_box_callback\n");

    MediaPlayerPrivateHelio *helioMediaPlayer = (MediaPlayerPrivateHelio *)ctx;

    //rcv_node_t *node = rcv_node_child(root, "pssh");
    rcv_pssh_box_t *pssh = node ? RCV_PSSH_BOX(rcv_node_raw(node)) : NULL;

    if (pssh && helioMediaPlayer) {
        uint8_t *systemId = NULL;
        uint8_t *initData = NULL;
        size_t initDataLength = 0;

        rcv_pssh_system_id(pssh, &systemId, NULL);
        rcv_raw_box_buffer(pssh, &initData, &initDataLength);

        helioMediaPlayer->sourceBufferDetectedEncryption(systemId, initData, initDataLength);
    }
}

static void rcv_media_read_callback_source_buffer(void *context) {
    // printf("SourceBufferPrivateHelio rcv_media_read_callback_source_buffer\n");
    SourceBufferPrivateHelio *sb = (SourceBufferPrivateHelio *)context;

    sb->platformBufferAvailable();
}


RefPtr<SourceBufferPrivateHelio> SourceBufferPrivateHelio::create(MediaPlayerPrivateHelio *player,
                                                                      MediaSourcePrivateHelio *parent,
                                                                      String codec) {
    // printf("SourceBufferPrivateHelio create\n");
    return adoptRef(new SourceBufferPrivateHelio(player, parent, codec));
}

// TODO: This needs to be fixed in librcvmf
// Since the listener was on the stack it was never called.
rcv_box_listener_t listeners[] = {
    { "pssh", rcv_pssh_box_callback },
    { NULL, NULL }
};

SourceBufferPrivateHelio::SourceBufferPrivateHelio(MediaPlayerPrivateHelio *player,
                                                   MediaSourcePrivateHelio *parent,
                                                   String codec)
  : m_readyState(MediaPlayer::HaveNothing)
  , m_weakFactory(this)
  , m_mediaPlayer(player)
  , m_mediaSource(parent)
  , m_notifyReadyForSample(false)
  , m_trackId() {
#if SBPH_PRINT
      printf("SourceBufferPrivateHelio constructor: %s\n", codec.utf8().data());
#endif

    m_asyncQueue = cvmf_queue_init();

    m_enqueuedSample = nullptr;

    // TODO: Create a "program" to play based on the content type and codec
    m_mediaPipeline = NULL;
    m_mediaStream = rcv_media_stream_init(rcv_media_read_callback_source_buffer,
                                          this,
                                          codec.utf8().data());
    // TODO: Don't pass the MediaPlayer here..
    m_rcvmfParser = rcv_parser_init(m_mediaPlayer, listeners);
}

SourceBufferPrivateHelio::~SourceBufferPrivateHelio() {
    printf("*******    SourceBufferPrivateHelio destructor    *******\n");
    // Moved to the removedFromSource call..
    // if (m_asyncQueue) {
    //     cvmf_queue_destroy(&m_asyncQueue);
    // }
    // if (m_rcvmfParser) {
    //     rcv_parser_destroy(&m_rcvmfParser);
    // }
}

void SourceBufferPrivateHelio::setClient(SourceBufferPrivateClient* client) {
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio setClient\n");
#endif
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
    //printf("SourceBufferPrivateHelio(%p) append size:%i\n", this, length);

    // struct timeval tv;
    // gettimeofday(&tv, NULL);
    //
    // unsigned long long start =
    //   (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    //
    // printf("---------------------------------------------- > SourceBufferPrivateHelio(%p) append START: %llu\n", this, start);

    bool appendSuccess = false;
    if (!m_client) {
        return;
    }

    // printf("SourceBufferPrivateHelio rcv_parse_isobmff(m_rcvmfParser, root, data, length) DONE;\n");
    rcv_node_t *root = rcv_node_init();
    rcv_parse_isobmff(m_rcvmfParser, root, data, length);

    // printf("SourceBufferPrivateHelio rcv_parse_isobmff(m_rcvmfParser, root, data, length) DONE;\n");

    if (rcv_node_child(root, "mdat")) {

        appendSuccess = this->didDetectISOBMFFMediaSegment(root);

        if (!appendSuccess) {
            rcv_destory_tree(&root);
        }

    } else if (rcv_node_child(root, "tkhd")) {
        // TODO: ^ this is sort a bad else if, moov & moof instead?

        // TODO: This could be more async.. clock this to determine.
        appendSuccess = this->didDetectISOBMFFInitSegment(root);

        // TODO: Test that the values we need have been copied out of this..
        rcv_destory_tree(&root);
    }
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio(%p) sourceBufferPrivateAppendComplete %s\n", this,
           appendSuccess ? "AppendSucceeded" : "ERROR: ParsingFailed");
#endif

    // gettimeofday(&tv, NULL);
    //
    // unsigned long long end =
    //   (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    // printf("SourceBufferPrivateHelio append END: %llu TOTAL: %llu\n", end, end - start);


    m_client->sourceBufferPrivateAppendComplete(this,
        appendSuccess ? SourceBufferPrivateClient::AppendSucceeded : SourceBufferPrivateClient::ParsingFailed);

    // gettimeofday(&tv, NULL);
    // unsigned long long total =
    //   (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    // printf("---------------------------------------------- >  SourceBufferPrivateHelio(%p) append TOTAL: %llu\n", this, total - start);
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
        // if (box) {
        //     printf("SourceBufferPrivateHelio Found STSD box\n");
        // } else {
        //     printf("SourceBufferPrivateHelio ERROR FINDING STSD box\n");
        // }

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

            // printf("SourceBufferPrivateHelio rcv_stsd_sample_entry_type... %s\n", format);

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

                SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
                vtInfo.track = VideoTrackPrivateHelio::create(id);
                vtInfo.description = m_trackDescription;
#if SBPH_PRINT
                printf("SourceBufferPrivateHelio VideoTrackPrivateHelio m_id:%s\n", vtInfo.track->id().string().utf8().data());
#endif
                m_trackId = vtInfo.track->id();

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

                SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
                atInfo.track = AudioTrackPrivateHelio::create(id);
                atInfo.description = m_trackDescription;
#if SBPH_PRINT
                printf("SourceBufferPrivateHelio AudioTrackPrivateHelio m_id:%s\n", atInfo.track->id().string().utf8().data());
#endif
                m_trackId = atInfo.track->id();
                segment.audioTracks.append(atInfo);
            }

            // printf("SourceBufferPrivateHelio sourceBufferPrivateDidReceiveInitializationSegment\n");
            m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);
            reportedInitSegment = true;

        }
    }

    return reportedInitSegment;

}

bool SourceBufferPrivateHelio::didDetectISOBMFFMediaSegment(rcv_node_t *root) {

    // printf("SourceBufferPrivateHelio (%p) didDetectISOBMFFMediaSegment\n", this);

    // TODO: Does this local var produce ref count churn?
    Ref<MediaSample> mediaSample = MediaSampleHelio::create(root,
                                                            m_timescale,
                                                            m_codecConfiguration);

    m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);

    return true;
}

void SourceBufferPrivateHelio::abort() {
    printf("ERROR SourceBufferPrivateHelio abort\n");
}

void SourceBufferPrivateHelio::resetParserState() {
    printf("ERROR TODO: SourceBufferPrivateHelio resetParserState\n");
}

void SourceBufferPrivateHelio::removedFromMediaSource() {
    printf("*******    SourceBufferPrivateHeli(%p)::removedFromMediaSource    *******\n", this);
    // TODO: Clean up some of the shit we've allocated here..

    if (m_mediaStream) {
        rcv_media_stream_detroy(&m_mediaStream);
    }
    if (m_mediaPipeline) {
        rcv_media_pipeline_destroy(&m_mediaPipeline);
    }

    if (m_asyncQueue) {
        cvmf_queue_destroy(&m_asyncQueue);
    }
    if (m_rcvmfParser) {
        rcv_parser_destroy(&m_rcvmfParser);
    }
}

MediaPlayer::ReadyState SourceBufferPrivateHelio::readyState() const {
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio readyState\n");
#endif
    return m_readyState;
}

void SourceBufferPrivateHelio::setReadyState(MediaPlayer::ReadyState readyState) {
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio setReadyState:%i\n", readyState);
#endif
    m_readyState = readyState;
}

void SourceBufferPrivateHelio::flush(AtomicString as) {
    printf("ERROR TODO: SourceBufferPrivateHelio flush:%s\n", as.string().utf8().data());
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

// Newer signature from WebKit
// void SourceBufferPrivateHelio::enqueueSample(Ref<MediaSample>&& sample, AtomicString as) {
void SourceBufferPrivateHelio::enqueueSample(PassRefPtr<MediaSample> mediaSample, AtomicString as) {
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio(%p) enqueueSample: %s\n", this, as.string().utf8().data());
#endif

    m_enqueuedSample = static_cast<MediaSampleHelio *>(mediaSample.get());

    // m_trackDecription doesn't work well here..
    // if (m_enqueuedSample->isAudio()) {
    //     printf("ERROR SourceBufferPrivate enqueueSample with decodeTime:%f\n", m_enqueuedSample->decodeTime().toDouble());
    // }

    if (m_enqueuedSample->isEncrypted()) {

        PlayreadySession::playreadyTask(2, [this](PlayreadySession *prSession){
#if SBPH_PRINT
            printf("SourceBufferPrivateHelio::enqueueSample decrypt task START\n");
#endif

            uint64_t start = now();
            m_enqueuedSample->decryptBuffer([prSession](const void* iv, uint32_t ivSize, void* payloadData, uint32_t payloadDataSize) -> int {
                // TODO: If this fails we need to stop processing this buffer..
                prSession->processPayload(iv, ivSize, payloadData, payloadDataSize);
            });
            uint64_t elapsed = now() - start;
            printf("Perf-Eric # Full Sample Decrypt time: %llu ms.\n", elapsed);

            cvmf_task_t *task = cvmf_task_init([](cvmf_async_queue_t *, void *ctx){
                SourceBufferPrivateHelio *helioSB = (SourceBufferPrivateHelio *)ctx;
                helioSB->writeEnqueuedSample();
            }, this);

            enqueue_task(m_asyncQueue, task);
#if SBPH_PRINT
            printf("SourceBufferPrivateHelio::enqueueSample decrypt task END\n");
#endif

        });

        return;
    }

    cvmf_task_t *task = cvmf_task_init([](cvmf_async_queue_t *, void *ctx){
        SourceBufferPrivateHelio *helioSB = (SourceBufferPrivateHelio *)ctx;
        helioSB->writeEnqueuedSample();
    }, this);
    enqueue_task(m_asyncQueue, task);

}

void SourceBufferPrivateHelio::writeEnqueuedSample() {
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio(%p)::writeEnqueuedSample()\n", this);
#endif

    // Pointer to the buffer we can fill with PES packets
    uint8_t *writeBuffer = NULL;
    // number of bytes available in the buffer we can fill
    size_t totalAvailable = 0;
    size_t bufferAvailable = 0;
    // rcv_media_stream_write(m_mediaStream, writeBuffer, writeSize);
    if (!rcv_media_stream_buffer(m_mediaStream, &writeBuffer, &totalAvailable)) {
        printf("ERROR SourceBufferPrivateHelio unable to get buffer from pool\n");
        //m_enqueuedSample = helioSample;
        return;
    }

    bufferAvailable = totalAvailable;

    // printf("SourceBufferPrivateHelio NEXUS buffer available:%zu next packet:%zu\n", bufferAvailable, m_enqueuedSample->sizeOfNextPESPacket());
    while (bufferAvailable >= m_enqueuedSample->sizeOfNextPESPacket()) {
        size_t writeSize = 0;
        if (!m_enqueuedSample->writeNextPESPacket(&writeBuffer, &writeSize)) {
            // printf("SourceBufferPrivateHelio::enqueueSample loop break writeNextPESPacket writeSize: %zu\n", writeSize);
            break;
        }
        bufferAvailable -= writeSize;
        writeBuffer += writeSize;
//        printf("SourceBufferPrivateHelio::enqueueSample writeNextPESPacket:%zu\n", writeSize);
    }

    // We've got more samples to write, but probably ran out of space.
    if (m_enqueuedSample->sizeOfNextPESPacket()) {
#if SBPH_PRINT
        printf("SourceBufferPrivateHelio::enqueueSample WARN Data to write in this sample, but no more space in the buffer, m_enqueuedSample->sizeOfNextPacket(): %zu\n", m_enqueuedSample->sizeOfNextPESPacket());
#endif
        // m_enqueuedSample = helioSample;
    } else {
        m_enqueuedSample = nullptr;
    }


    // DEBUG
    // hex_dump_buffer(buffer, first_sample_size + pps_len + sps_len + 4);

    if (writeBuffer == NULL) {
        return;
    }

    //printf("SourceBufferPrivateHelio will write.. writeBuffer:%p\n", writeBuffer);

#if SBPH_PRINT
    // This check/print exists in rcvmf
    if (totalAvailable == bufferAvailable) {
        printf("SourceBufferPrivateHelio WARN NO DATA WRITTEN TO THE BUFFER\n");
    }
#endif

    // TODO: If the write is zero (the buffer is at the tail end and
    // doesn't have enough space for a packet) The buffer is skipped
    // but the engine immediately moves to the head of the ring buffer
    // and invokes the callback that the buffer is available.
    // because we currently are holding the lock with this task
    // the next task request becomes deadlocked.
    // This is an attempt to let the lock release so that if
    // the engine calls back in here we don't become deadlocked.
    // ideally the engine should wait to invoke the callback
    // and should be fixed in the engine layer.
    if (rcv_media_stream_write(m_mediaStream, totalAvailable - bufferAvailable)) {
#if SBPH_PRINT
        printf("SourceBufferPrivateHelio WRITE SUCCESS\n");
#endif
    } else {
        printf("ERROR: SourceBufferPrivateHelio WRITE ERROR\n");
    }

    // if (m_enqueuedSample == nullptr && m_notifyReadyForSample) {
    //     cvmf_task_t *task = cvmf_task_init([](void *ctx){
    //         SourceBufferPrivateHelio *helioSB = (SourceBufferPrivateHelio *)ctx;
    //         helioSB->queueClientNotification();
    //     }, this);
    //
    //     enqueue_task(m_asyncQueue, task);
    // }
}

// SourceBuffer calls this method twice before calling the notify method.
// Once as soon as the sample is appended, and then a second time after calling
// append done. Make sure the event is fired before returning true here
// We don't want to block the event from dispatching.
bool SourceBufferPrivateHelio::isReadyForMoreSamples(AtomicString as) {
    // printf("SourceBufferPrivateHelio (%p) isReadyForMoreSamples TRACK:%s %i\n",
    //        this, as.string().utf8().data(), m_enqueuedSample == nullptr);

    return !m_enqueuedSample;
}

// NOTE(estobb200): `setActive` does not get called with a false value during
// it's removal. This only happens during track changes.
void SourceBufferPrivateHelio::setActive(bool active) {
    // At this point we should assume we're going to need an a/v pipeline
    // for this content.
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio setActive:%i\n", active);
#endif

    if (!active || m_mediaPipeline) {
        printf("WARN: SourceBufferPrivateHelio setActive:%i\n", active);
        return;
    }

    cvmf_task_t *task = cvmf_task_init([](cvmf_async_queue_t *, void *ctx) {
        SourceBufferPrivateHelio *helioSB = (SourceBufferPrivateHelio *)ctx;
        helioSB->preparePipeline();
    }, this);

    enqueue_task(m_asyncQueue, task);

    // preparePipeline in the callback instead..
    //preparePipeline();
}

void SourceBufferPrivateHelio::preparePipeline() {
    if (m_mediaStream) {
#if SBPH_PRINT
        printf("SourceBufferPrivateHelio creating pipeline\n");
#endif

        m_mediaPipeline = rcv_media_pipeline_init(m_mediaSource->mediaPlatform(),
                                                  m_mediaStream,
                                                  m_mediaPlayer->platformClockController());

        // This is already in a task..
        startStream();
    }

    // Used to proxy back to the HTMLMediaElement for it to check if the
    // bSourceBuffer has audio or videio..
    WeakPtr<SourceBufferPrivateHelio> weakThis = createWeakThis();
    callOnMainThread([weakThis]{
        if (weakThis) {
            weakThis->reportActiveBuffer();
        }
    });
}

void SourceBufferPrivateHelio::reportActiveBuffer() const {
    m_mediaSource->sourceBufferPrivateActiveStateChanged();
}

void SourceBufferPrivateHelio::notifyClientWhenReadyForMoreSamples(AtomicString trackId) {
    // printf("SourceBufferPrivateHelio notifyClientWhenReadyForMoreSamples: %s \n", trackId.string().utf8().data());
    m_notifyReadyForSample = true;
}

// Called by the MediaSource as soon as all it's buffers become active
// and the pipelines are connected.
void SourceBufferPrivateHelio::startStream() {
#if SBPH_PRINT
    printf("SourceBufferPrivateHelio::startStream()\n");
#endif
    // TODO: at this point we MAY be ready for samples, but we wait until
    // the stream's data ready callback occurs.

    struct stream_init {
        rcv_media_stream_t *mediaStream;
        rcv_media_pipeline_t *mediaPipeline;
    };
    struct stream_init *sbStream = malloc(sizeof(struct stream_init));
    sbStream->mediaStream = m_mediaStream;
    sbStream->mediaPipeline = m_mediaPipeline;

    cvmf_task_t *task = cvmf_task_init([](cvmf_async_queue_t *, void *ctx) {
        struct stream_init *init = (struct stream_init *)ctx;
        rcv_media_pipeline_start_stream(init->mediaPipeline,
                                        init->mediaStream);
        free(init);
    }, sbStream);

    enqueue_task(m_asyncQueue, task);

}

void SourceBufferPrivateHelio::queueClientNotification() {
    m_notifyReadyForSample = false;
    // printf("SourceBufferPrivateHelio::platformBufferAvailable(), m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples\n");
    m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples(this, m_trackId);
}

// callback from rcvmf, do NOT do a lot of work in this call.
void SourceBufferPrivateHelio::platformBufferAvailable() {
    // printf("SourceBufferPrivateHelio::platformBufferAvailable()\n");
    if (m_enqueuedSample != nullptr) {
        // If we've enqueued it, but it's encrypted means it already has a pending task..
        if (m_enqueuedSample->isEncrypted()) {
            return;
        }

        // printf("SourceBufferPrivateHelio::platformBufferAvailable(), enqueuing previous sample\n");

        cvmf_task_t *task = cvmf_task_init([](cvmf_async_queue_t *, void *ctx){
            SourceBufferPrivateHelio *helioSB = (SourceBufferPrivateHelio *)ctx;
            helioSB->writeEnqueuedSample();
        }, this);

        enqueue_task(m_asyncQueue, task);

        return;
    }

    if (m_notifyReadyForSample) {
        cvmf_task_t *task = cvmf_task_init([](cvmf_async_queue_t *, void *ctx){
            SourceBufferPrivateHelio *helioSB = (SourceBufferPrivateHelio *)ctx;
            helioSB->queueClientNotification();
        }, this);

        enqueue_task(m_asyncQueue, task);
    }


    // } else if (m_notifyReadyForSample) {
    //     m_notifyReadyForSample = false;
    //
    //     // TODO: Check if there's a difference between this and our async queue
    //     m_notifyQueue.enqueueTask([this]{
    //         // TODO: Try calling on mainThread..
    //         printf("SourceBufferPrivateHelio::platformBufferAvailable(), m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples\n");
    //         // This is a couple hundred ms call, we're using the platform engine
    //         // thread here.
    //         struct timeval tv;
    //         gettimeofday(&tv, NULL);
    //
    //         unsigned long long start =
    //           (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    //
    //         m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples(this, m_trackId);
    //
    //         gettimeofday(&tv, NULL);
    //
    //         unsigned long long end =
    //           (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    //
    //         printf("SourceBufferPrivateHelio sourceBufferPrivateDidBecomeReadyForMoreSamples TOTAL: %llu\n", end - start);
    //     });
    //
    // }
}

}
