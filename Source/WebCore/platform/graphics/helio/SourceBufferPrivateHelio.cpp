//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "SourceBufferPrivateHelio.h"
#include "MediaSampleHelio.h"
#include "VideoTrackPrivateHelio.h"
#include "AudioTrackPrivateHelio.h"
#include "MediaSourcePrivateHelio.h"
#include "MediaPlayerPrivateHelio.h"
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
    
static size_t rcv_media_read_callback_source_buffer(void *context __attribute__((unused)),
                                                    void *buffer __attribute__((unused)),
                                                    size_t buffer_size) {
    printf("SourceBufferPrivateHelio READ TO WRITE: %zu \n", buffer_size);
    return 0;
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
  , m_mediaSource(parent) {
      printf("SourceBufferPrivateHelio constructor: %s\n", codec.utf8().data());
      // TODO: Fix this in helio..
      //   static const hve_event_callback_t callbacks[] = {
      //     { hve_ready_event,  hve_ready_handler },
      //     { hve_track_info_event,  hve_track_info_handler },
      //     { hve_media_sample_event, hve_media_sample_handler },
      //     { hve_demux_start_event, hve_demux_start_handler },
      //     { hve_demux_complete_event, hve_demux_complete_handler },
      //     { NULL, NULL }
      //   };
      //   if (!m_helio) {
      //       m_helio = helio_init(this, callbacks);
      //   }

    // NOTE(estobb200): We do two things here.. the callbacks only listen
    // for isobmff init segmenets, once it's detected, we report it back.
    // static const hve_event_callback_t callbacks[] = {
    //     { hve_ready_event,  hve_ready_handler },
    //     { hve_track_info_event,  hve_track_info_handler },
    //     { hve_media_sample_event, hve_media_sample_handler },
    //     { hve_demux_start_event, hve_demux_start_handler },
    //     { hve_demux_complete_event, hve_demux_complete_handler },
    //     { NULL, NULL }
    //   };

    // static const rcv_box_listener_t callbacks[] = {
    //     { "ftyp", _ftyp_box_callback },
    //     { "frma", _frma_box_callback },
    //     { "stts", _track_box_callback },
    //     { "stsc", _track_box_callback },
    //     { "stco", _track_box_callback },
    //     { "mvex", _mvex_box_callback },
    //     // { "tkhd", _tkhd_box_callback },
    //     { NULL, NULL }
    // };
      
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

    printf("rcv_parse_isobmff(m_rcvmfParser, root, data, length) DONE;\n");

    if (rcv_node_child(root, "mdat")) {
        
        appendSuccess = this->didDetectISOBMFFMediaSegment(root);
        
        if (!appendSuccess) {
            rcv_destory_tree(&root);
        }

        printf("sourceBufferPrivateAppendComplete SAMPLES");

    } else if (rcv_node_child(root, "tkhd")) {
        // TODO: ^ this is sort a bad else if

        appendSuccess = this->didDetectISOBMFFInitSegment(root);
        
        rcv_destory_tree(&root);
    }
    
    printf("sourceBufferPrivateAppendComplete %s\n", appendSuccess ? "AppendSucceeded" : "ParsingFailed");
    
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
        box = rcv_node_first_child(box);
        
        char *s = rcv_node_type(box);
        
        m_trackDescription = MediaDescriptionHelio::create(strcmp(s, "avcC") == 0 ? "avc1" : "mp4a");
        
        rcv_node_t *child_box = rcv_node_child(root, "hdlr");
        if (child_box) {
            rcv_hdlr_box_t *hdlr = RCV_HDLR_BOX(rcv_node_raw(child_box));
            
            // TODO: Audio samples.. Video Samples flags..
            char *format = rcv_hdlr_handler_type(hdlr);
            
            printf("rcv_stsd_sample_entry_type... %s\n", format);
            
            if (strcmp(format, "vide") == 0) {
                SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
                vtInfo.track = VideoTrackPrivateHelio::create(id);
                vtInfo.description = m_trackDescription;
                printf("VideoTrackPrivateHelio m_id:%s\n", vtInfo.track->id().string().utf8().data());
                
                segment.videoTracks.append(vtInfo);
            } else if (strcmp(format, "soun") == 0) {
                SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
                atInfo.track = AudioTrackPrivateHelio::create(id);
                atInfo.description = m_trackDescription;
                
                printf("AudioTrackPrivateHelio m_id:%s\n", atInfo.track->id().string().utf8().data());
                segment.audioTracks.append(atInfo);
            }
            
            
            printf("sourceBufferPrivateDidReceiveInitializationSegment\n");
            m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);
            reportedInitSegment = true;
            
        }
    }
    
    return reportedInitSegment;

}

bool SourceBufferPrivateHelio::didDetectISOBMFFMediaSegment(rcv_node_t *root) {

    Ref<MediaSample> mediaSample = MediaSampleHelio::create(root, m_timescale);

    printf("sourceBufferPrivateDidReceiveSample\n");
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
    printf("SourceBufferPrivateHelio flush:%s\n", as.string().utf8().data());
}

void SourceBufferPrivateHelio::enqueueSample(PassRefPtr<MediaSample>, AtomicString as) {
    printf("SourceBufferPrivateHelio enqueueSample:%s\n", as.string().utf8().data());
    //TODO: Write data
}

// SourceBuffer calls this method twice before calling the notify method.
// Once as soon as the sample is appended, and then a second time after calling
// append done. Make sure the event is fired before returning true here
// We don't want to block the event from dispatching.
bool SourceBufferPrivateHelio::isReadyForMoreSamples(AtomicString as) {
    printf("SourceBufferPrivateHelio isReadyForMoreSamples TRACK:%s \n",
           as.string().utf8().data());

    return false;
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

    m_mediaSource->sourceBufferPrivateActiveStateChanged(this, m_mediaPipeline);

}

void SourceBufferPrivateHelio::notifyClientWhenReadyForMoreSamples(AtomicString trackId) {
    printf("SourceBufferPrivateHelio notifyClientWhenReadyForMoreSamples:%s\n", trackId.string().utf8().data());
}
    
//void SourceBufferPrivateHelio::writeBufferAvailable() {
//    //m_readyForMoreSamples = true;
//}
    
// Called by the MediaSource as soon as all it's buffers become active
// and the pipelines are connected.
void SourceBufferPrivateHelio::startStream() {
    // TODO: at this point we MAY be ready for samples, but we wait until
    // the stream's data ready callback occurs.
    rcv_media_pipeline_start_stream(m_mediaPipeline, m_mediaStream);
}

}
