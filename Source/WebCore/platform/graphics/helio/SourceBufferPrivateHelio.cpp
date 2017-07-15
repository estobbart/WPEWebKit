//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "SourceBufferPrivateHelio.h"
#include "MediaSampleHelio.h"
#include "MediaDescription.h"
#include "VideoTrackPrivateHelio.h"
#include "AudioTrackPrivateHelio.h"
//#include "InbandTextTrackPrivate.h"


#include <stdio.h>
#include <string.h> // memcpy

#import <wtf/MainThread.h>

namespace WebCore {

class MediaDescriptionHelio final : public MediaDescription {
public:
    static RefPtr<MediaDescriptionHelio> create(helio_track_t *track) { return adoptRef(new MediaDescriptionHelio(track)); }
    virtual ~MediaDescriptionHelio() { }

    AtomicString codec() const override { return m_codec; }
    bool isVideo() const override { return m_isVideo; }
    bool isAudio() const override { return m_isAudio; }
    bool isText() const override { return m_isText; }

protected:
    MediaDescriptionHelio(helio_track_t *track)
        : m_isVideo(track->type == hve_track_video)
        , m_isAudio(track->type == hve_track_audio)
        , m_isText(track->type == hve_track_text)
    {
        // NSArray* formatDescriptions = [track formatDescriptions];
        // CMFormatDescriptionRef description = [formatDescriptions count] ? (CMFormatDescriptionRef)[formatDescriptions objectAtIndex:0] : 0;
        // if (description) {
        //     FourCharCode codec = CMFormatDescriptionGetMediaSubType(description);
        //     m_codec = AtomicString(reinterpret_cast<LChar*>(&codec), 4);
        // }
        m_codec = AtomicString(track->codec);
    }

    AtomicString m_codec;
    bool m_isVideo;
    bool m_isAudio;
    bool m_isText;
};


// TODO: Move all these to calls on the main thread..
static void hve_ready_handler(void *sourceBufferPrivate __attribute__((unused)),
                                void *event_data __attribute__((unused))) {
    printf("helio_ready_handler\n");
}

static void hve_track_info_handler(void *sourceBufferPrivate, void *event_data) {
    /**
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
     */
//    printf("hve_track_info_handler\n");
    SourceBufferPrivateHelio *sbHelio = (SourceBufferPrivateHelio *)sourceBufferPrivate;
    helio_track_info_t * track_info = (helio_track_info_t *)event_data;
    helio_track_t **tracks = track_info->tracks;
    uint8_t num_tracks = track_info->track_count;

    AtomicString t1 = AtomicString::number(100);

    callOnMainThread([sbHelio, tracks, num_tracks] {


        SourceBufferPrivateClient::InitializationSegment segment;
        segment.duration = MediaTime(); //TODO: duration of what? the data received?

        uint8_t idx = 0;
        printf("number of tracks:%i\n", num_tracks);
        for (;idx < num_tracks; idx++) {
            printf("tracks[idx].type:%i\n", tracks[idx]->type);
            if (tracks[idx]->type == hve_track_video) {
                printf("hve_track_info_handler hve_track_video\n");
                SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
                vtInfo.track = VideoTrackPrivateHelio::create(tracks[idx]);
                vtInfo.description = MediaDescriptionHelio::create(tracks[idx]);
                printf("VideoTrackPrivateHelio m_id:%s\n", vtInfo.track->id().string().utf8().data());

                segment.videoTracks.append(vtInfo);

            } else if (tracks[idx]->type == hve_track_audio) {

                printf("hve_track_info_handler hve_track_audio\n");
                SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
                atInfo.track = AudioTrackPrivateHelio::create(tracks[idx]);
                atInfo.description = MediaDescriptionHelio::create(tracks[idx]);

                printf("AudioTrackPrivate m_id:%s\n", atInfo.track->id().string().utf8().data());
                segment.audioTracks.append(atInfo);
            }
        }



        if (sbHelio) {
            sbHelio->trackInfoEventHandler(segment);
        }
        //delete segment;
    });
}

static void hve_media_sample_handler(void *sourceBufferPrivate, void *event_data) {
//    printf("hve_media_sample_handler\n");
    SourceBufferPrivateHelio *sbHelio = (SourceBufferPrivateHelio *)sourceBufferPrivate;
    helio_sample_t ** samples = (helio_sample_t **)event_data;
    //memcpy(sample, event_data, sizeof(helio_sample_t));
    callOnMainThread([sbHelio, samples] {
        if (sbHelio) {
            sbHelio->mediaSampleEventHandler(samples);
        }
    });
}

static void hve_demux_start_handler(void *sourceBufferPrivate __attribute__((unused)),
                                    void *event_data __attribute__((unused))) {
    printf("hve_demux_start_handler\n");
}

static void hve_demux_complete_handler(void *sourceBufferPrivate,
                                       void *event_data __attribute__((unused))) {
    printf("hve_demux_complete_handler\n");
    SourceBufferPrivateHelio *sbHelio = (SourceBufferPrivateHelio *)sourceBufferPrivate;
    callOnMainThread([sbHelio] {
        if (sbHelio)
            sbHelio->demuxCompleteEventHandler();
    });
}

RefPtr<SourceBufferPrivateHelio> SourceBufferPrivateHelio::create(MediaSourcePrivateHelio* parent) {
    printf("SourceBufferPrivateHelio create\n");
    return adoptRef(new SourceBufferPrivateHelio(parent));
}

SourceBufferPrivateHelio::SourceBufferPrivateHelio(MediaSourcePrivateHelio* parent __attribute__((unused)))
  : m_readyState(MediaPlayer::HaveNothing) {
      printf("SourceBufferPrivateHelio constructor\n");
      // TODO: Fix this in helio..
      static const hve_event_callback_t callbacks[] = {
        { hve_ready_event,  hve_ready_handler },
        { hve_track_info_event,  hve_track_info_handler },
        { hve_media_sample_event, hve_media_sample_handler },
        { hve_demux_start_event, hve_demux_start_handler },
        { hve_demux_complete_event, hve_demux_complete_handler },
        { NULL, NULL }
      };
      if (!m_helio) {
          m_helio = helio_init(this, callbacks);
      }
}

SourceBufferPrivateHelio::~SourceBufferPrivateHelio() {
    printf("SourceBufferPrivateHelio destructor\n");
    if (m_helio) {
        helio_destroy(m_helio);
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
 */
void SourceBufferPrivateHelio::append(const unsigned char* data,
                                      unsigned length) {
    printf("SourceBufferPrivateHelio append size:%i\n", length);

    helio_enque_data(m_helio, data, length);
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

bool SourceBufferPrivateHelio::isReadyForMoreSamples(AtomicString as) {
    printf("SourceBufferPrivateHelio isReadyForMoreSamples:%s true\n", as.string().utf8().data());
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
    return false;
}

void SourceBufferPrivateHelio::setActive(bool active) {
    printf("SourceBufferPrivateHelio setActive:%i\n", active);
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

void SourceBufferPrivateHelio::trackInfoEventHandler(const SourceBufferPrivateClient::InitializationSegment &segment) {
    printf("SourceBufferPrivateHelio trackInfoEventHandler\n");

    if (m_client) {
        m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);
    }
}

void SourceBufferPrivateHelio::mediaSampleEventHandler(helio_sample_t **samples) {
    // printf("SourceBufferPrivateHelio mediaSampleEventHandler\n");
    if (m_client) {
        while (samples != NULL) {
            Ref<MediaSample> mediaSample = MediaSampleHelio::create(*samples);
            m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);
            samples++;
        }
    }
}

void SourceBufferPrivateHelio::demuxCompleteEventHandler() {
    printf("SourceBufferPrivateHelio demuxCompleteEventHandler\n");
    if (m_client) {
        // TODO: Or SourceBufferPrivateClient::ParsingFailed
        m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);
    }
}


}
