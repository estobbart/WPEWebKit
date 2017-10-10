//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "SourceBufferPrivateHelio.h"
#include "MediaSampleHelio.h"
#include "VideoTrackPrivateHelio.h"
#include "AudioTrackPrivateHelio.h"
#include "MediaSourcePrivateHelio.h"
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


// static void hve_track_info_handler(void *sourceBufferPrivate, void *event_data) {

//      */
// //    printf("hve_track_info_handler\n");
//     SourceBufferPrivateHelio *sbHelio = (SourceBufferPrivateHelio *)sourceBufferPrivate;
//     helio_track_info_t * track_info = (helio_track_info_t *)event_data;
//     helio_track_t **tracks = track_info->tracks;
//     uint8_t num_tracks = track_info->track_count;
//
//     AtomicString t1 = AtomicString::number(100);
//
//     callOnMainThread([sbHelio, tracks, num_tracks] {
//
//
//         SourceBufferPrivateClient::InitializationSegment segment;
//         segment.duration = MediaTime(); //TODO: duration of what? the data received?
//
//         uint8_t idx = 0;
//         printf("number of tracks:%i\n", num_tracks);
//         for (;idx < num_tracks; idx++) {
//             printf("tracks[idx].type:%i\n", tracks[idx]->type);
//             if (tracks[idx]->type == hve_track_video) {
//                 printf("hve_track_info_handler hve_track_video\n");
//                 SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
//                 vtInfo.track = VideoTrackPrivateHelio::create(tracks[idx]);
//                 vtInfo.description = MediaDescriptionHelio::create(tracks[idx]);
//                 printf("VideoTrackPrivateHelio m_id:%s\n", vtInfo.track->id().string().utf8().data());
//
//                 segment.videoTracks.append(vtInfo);
//
//             } else if (tracks[idx]->type == hve_track_audio) {
//
//                 printf("hve_track_info_handler hve_track_audio\n");
//                 SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
//                 atInfo.track = AudioTrackPrivateHelio::create(tracks[idx]);
//                 atInfo.description = MediaDescriptionHelio::create(tracks[idx]);
//
//                 printf("AudioTrackPrivate m_id:%s\n", atInfo.track->id().string().utf8().data());
//                 segment.audioTracks.append(atInfo);
//             }
//         }
//
//
//
//         if (sbHelio) {
//             sbHelio->trackInfoEventHandler(segment);
//         }
//         //delete segment;
//     });
// }

// static void hve_media_sample_handler(void *sourceBufferPrivate, void *event_data) {
// //    printf("hve_media_sample_handler\n");
//     SourceBufferPrivateHelio *sbHelio = (SourceBufferPrivateHelio *)sourceBufferPrivate;
//     helio_sample_t ** samples = (helio_sample_t **)event_data;
//     //memcpy(sample, event_data, sizeof(helio_sample_t));
//     callOnMainThread([sbHelio, samples] {
//         if (sbHelio) {
//             sbHelio->mediaSampleEventHandler(samples);
//         }
//     });
// }

// static void hve_demux_start_handler(void *sourceBufferPrivate __attribute__((unused)),
//                                     void *event_data __attribute__((unused))) {
//     printf("hve_demux_start_handler\n");
// }

// static void hve_demux_complete_handler(void *sourceBufferPrivate,
//                                        void *event_data __attribute__((unused))) {
//     printf("hve_demux_complete_handler\n");
//     SourceBufferPrivateHelio *sbHelio = (SourceBufferPrivateHelio *)sourceBufferPrivate;
//     callOnMainThread([sbHelio] {
//         if (sbHelio)
//             sbHelio->demuxCompleteEventHandler();
//     });
// }

RefPtr<SourceBufferPrivateHelio> SourceBufferPrivateHelio::create(MediaSourcePrivateHelio* parent) {
    printf("SourceBufferPrivateHelio create\n");
    return adoptRef(new SourceBufferPrivateHelio(parent));
}

SourceBufferPrivateHelio::SourceBufferPrivateHelio(MediaSourcePrivateHelio* parent)
  : m_mediaSource(parent)
  , m_weakFactory(this)
  , m_readyState(MediaPlayer::HaveNothing)
  , m_readyForMoreSamples(true) {
      printf("SourceBufferPrivateHelio constructor\n");
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

    if (!m_client) {
        return;
    }

    // WeakPtr<SourceBufferPrivateHelio> weakThis = createWeakPtr();
    //helio_enque_data(m_helio, data, length);

    rcv_node_t *root = rcv_node_init();
    rcv_parse_isobmff(m_rcvmfParser, root, data, length);
    rcv_node_t *box;

    printf("rcv_parse_isobmff(m_rcvmfParser, root, data, length) DONE;\n");

    if (rcv_node_child(root, "mdat")) {
        // callOnMainThread([weakThis, root] {
        //     weakThis->didDetectISOBMFFSegment(root);
        // });

        Ref<MediaSample> mediaSample = MediaSampleHelio::create(root, m_timescale);

        printf("sourceBufferPrivateDidReceiveSample\n");
        m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);

        printf("sourceBufferPrivateAppendComplete SAMPLES\n");
        m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);

        return;
    }

    box = rcv_node_child(root, "tkhd");
    if (box) {
        // callOnMainThread([weakThis, root] {
        //     weakThis->didDetectISOBMFFHeader(root);
        // });
      
        SourceBufferPrivateClient::InitializationSegment segment;
      
        rcv_tkhd_box_t *tkhd = RCV_TKHD_BOX(rcv_node_raw(box));

      
        // printf("rcv_tkhd_track_id .... \n");
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

        box = rcv_node_child(root, "stsd");
        box = rcv_node_first_child(box);
        char *codec = rcv_node_type(box);

        // printf("rcv_node_child .... \n");
        rcv_node_t *child_box = rcv_node_child(root, "hdlr");
        if (child_box) {
            // printf("rcv_node_raw hdlr .... \n");
            rcv_hdlr_box_t *hdlr = RCV_HDLR_BOX(rcv_node_raw(child_box));
            // printf("format request \n");

            // TODO: Audio samples.. Video Samples flags..
            char *format = rcv_hdlr_handler_type(hdlr);

            printf("rcv_stsd_sample_entry_type... %s\n", format);

            if (strcmp(format, "vide") == 0) {
                SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
                vtInfo.track = VideoTrackPrivateHelio::create(id);
                vtInfo.description = MediaDescriptionHelio::create(codec);
                printf("VideoTrackPrivateHelio m_id:%s\n", vtInfo.track->id().string().utf8().data());

                segment.videoTracks.append(vtInfo);
            } else if (strcmp(format, "soun") == 0) {
                SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
                atInfo.track = AudioTrackPrivateHelio::create(id);
                atInfo.description = MediaDescriptionHelio::create(codec);

                printf("AudioTrackPrivateHelio m_id:%s\n", atInfo.track->id().string().utf8().data());
                segment.audioTracks.append(atInfo);
            }


            printf("sourceBufferPrivateDidReceiveInitializationSegment\n");
            m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);

            printf("sourceBufferPrivateAppendComplete\n");
            m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);

        } else {
            printf("sourceBufferPrivateAppendComplete FAILED!!\n");
            m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::ParsingFailed);
        }

        rcv_destory_tree(&root);

        return;
    }

    // TODO: Move this to the main thread..
    printf("sourceBufferPrivateAppendComplete ParsingFailed\n");
    //m_client->sourceBufferPrivateAppendComplete(weakThis, SourceBufferPrivateClient::ParsingFailed);

}

void SourceBufferPrivateHelio::didDetectISOBMFFHeader(rcv_node_t *root) {
    // printf("initSegment.... \n");
    // init segment
    rcv_node_t *box = rcv_node_child(root, "tkhd");
    rcv_tkhd_box_t *tkhd = RCV_TKHD_BOX(rcv_node_raw(box));

    // printf("segment .... \n");
    SourceBufferPrivateClient::InitializationSegment segment;
    // TODO: Theres a few different durations, but we don't use that value
    // in the live segments.
    // printf("MediaTime .... \n");
    segment.duration = MediaTime();
    // printf("rcv_tkhd_track_id .... \n");
    uint32_t id = rcv_tkhd_track_id(tkhd);

    // printf("rcv_node_child .... \n");
    rcv_node_t *child_box = rcv_node_child(root, "hdlr");
    if (child_box) {
        // printf("rcv_node_raw hdlr .... \n");
        rcv_hdlr_box_t *hdlr = RCV_HDLR_BOX(rcv_node_raw(child_box));
        // printf("format request \n");

        // TODO: Audio samples.. Video Samples flags..
        char *format = rcv_hdlr_handler_type(hdlr);

        printf("rcv_stsd_sample_entry_type... %s\n", format);

        if (strcmp(format, "vide") == 0) {
            SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
            vtInfo.track = VideoTrackPrivateHelio::create(id);
            //vtInfo.description = m_trackDescription;
            printf("VideoTrackPrivateHelio m_id:%s\n", vtInfo.track->id().string().utf8().data());

            segment.videoTracks.append(vtInfo);
        } else if (strcmp(format, "soun") == 0) {
            SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
            atInfo.track = AudioTrackPrivateHelio::create(id);
            //atInfo.description = m_trackDescription;

            printf("AudioTrackPrivateHelio m_id:%s\n", atInfo.track->id().string().utf8().data());
            segment.audioTracks.append(atInfo);
        }


        printf("sourceBufferPrivateDidReceiveInitializationSegment\n");
        m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);

        printf("sourceBufferPrivateAppendComplete\n");
        m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);

    } else {
        printf("sourceBufferPrivateAppendComplete FAILED!!\n");
        m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::ParsingFailed);
    }

    rcv_destory_tree(&root);

}

void SourceBufferPrivateHelio::didDetectISOBMFFSegment(rcv_node_t *root) {
    // samples..
    uint32_t timescale = 90000;
//    rcv_node_t *box = rcv_node_child(root, "mvhd");
//    if (box) {
//        rcv_mvhd_box_t *mvhd = RCV_MVHD_BOX(rcv_node_raw(box));
//        timescale = rcv_mvhd_timescale(mvhd);
//    }

    Ref<MediaSample> mediaSample = MediaSampleHelio::create(root, timescale);

    printf("sourceBufferPrivateDidReceiveSample");
    m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);

    printf("sourceBufferPrivateAppendComplete SAMPLES");
    m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);
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
}

// SourceBuffer calls this method twice before calling the notify method.
// Once as soon as the sample is appended, and then a second time after calling
// append done. Make sure the event is fired before returning true here
// We don't want to block the event from dispatching.
bool SourceBufferPrivateHelio::isReadyForMoreSamples(AtomicString as) {
    m_readyForMoreSamples = !m_readyForMoreSamples;
    printf("SourceBufferPrivateHelio isReadyForMoreSamples TRACK:%s m_readyForMoreSamples: %i \n",
           as.string().utf8().data(), m_readyForMoreSamples);
    /* if true...
        trackBuffer.decodeQueue.empty(): 0
        SourceBufferPrivateHelio isReadyForMoreSamples:256 true
        m_private->isReadyForMoreSamples(trackID) bool: 1
        SourceBufferPrivateHelio isReadyForMoreSamples:256 true
        SourceBufferPrivateHelio enqueueSample:256
        SourceBufferPrivateHelio isReadyForMoreSamples:256 true
        m_private->isReadyForMoreSamples(trackID) bool: 1
        SourceBufferPrivateHelio isReadyForMoreSamples:256 true
        trackBuffer.decodeQueue.empty(): 0
        SourceBufferPrivateHelio isReadyForMoreSamples:257 true
        m_private->isReadyForMoreSamples(trackID) bool: 1
        SourceBufferPrivateHelio isReadyForMoreSamples:257 true
        SourceBufferPrivateHelio enqueueSample:257
        SourceBufferPrivateHelio isReadyForMoreSamples:257 true
        m_private->isReadyForMoreSamples(trackID) bool: 1
        SourceBufferPrivateHelio isReadyForMoreSamples:257 true
        Until the buffer empties..

        if false..

        Pre queue trackBuffer(256).decodeQueue.empty(): 0
        SourceBufferPrivateHelio isReadyForMoreSamples:256 true
        SourceBufferPrivateHelio notifyClientWhenReadyForMoreSamples:256
        Post queue trackBuffer(256).decodeQueue.empty(): 0
        Pre queue trackBuffer(257).decodeQueue.empty(): 0
        SourceBufferPrivateHelio isReadyForMoreSamples:257 true
        SourceBufferPrivateHelio notifyClientWhenReadyForMoreSamples:257
        Post queue trackBuffer(257).decodeQueue.empty(): 0
    */
    return m_readyForMoreSamples;
}

void SourceBufferPrivateHelio::setActive(bool active) {
    // TODO: At this point we should assume we're going to need a a/v pipeline
    // for this content.
    printf("SourceBufferPrivateHelio setActive:%i\n", active);

    m_mediaSource->sourceBufferPrivateActiveStateChanged(active);
}

void SourceBufferPrivateHelio::notifyClientWhenReadyForMoreSamples(AtomicString trackId) {
    printf("SourceBufferPrivateHelio notifyClientWhenReadyForMoreSamples:%s\n", trackId.string().utf8().data());
    // auto it = m_trackNotifyMap.find(trackId);
    // if (it == m_trackNotifyMap.end()) {
    //     m_trackNotifyMap.add(trackId, true);
    // } else {
    //     it.value = true;
    // }
}

// void SourceBufferPrivateHelio::setTrackDescription(PassRefPtr<MediaDescriptionHelio> descryption) {
//     m_trackDescription = descryption;
// }

// void SourceBufferPrivateHelio::trackInfoEventHandler(const SourceBufferPrivateClient::InitializationSegment &segment) {
//     printf("SourceBufferPrivateHelio trackInfoEventHandler\n");
//
//     if (m_client) {
//         m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);
//     }
// }
//
// void SourceBufferPrivateHelio::mediaSampleEventHandler(helio_sample_t **samples) {
//     // printf("SourceBufferPrivateHelio mediaSampleEventHandler\n");
//     if (m_client) {
//         while (*samples != NULL) {
//             Ref<MediaSample> mediaSample = MediaSampleHelio::create(*samples);
//             m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);
//             samples++;
//         }
//     }
// }
//
// void SourceBufferPrivateHelio::demuxCompleteEventHandler() {
//     printf("SourceBufferPrivateHelio demuxCompleteEventHandler\n");
//     if (m_client) {
//         // TODO: Or SourceBufferPrivateClient::ParsingFailed
//         m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);
//     }
// }


}
