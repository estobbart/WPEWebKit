//if you use new/delete make sure to include config.h at the top of the file
#include "config.h"

#include "SourceBufferPrivateHelio.h"
#include "MediaSampleHelio.h"
#include "MediaDescription.h"
//#include "InbandTextTrackPrivate.h"

#include <stdio.h>
#include <string.h> // memcpy

namespace WebCore {

class MediaDescriptionHelio final : public MediaDescription {
public:
    static RefPtr<MediaDescriptionHelio> create() { return adoptRef(new MediaDescriptionHelio()); }
    virtual ~MediaDescriptionHelio() { }

    AtomicString codec() const override { return m_codec; }
    bool isVideo() const override { return m_isVideo; }
    bool isAudio() const override { return m_isAudio; }
    bool isText() const override { return m_isText; }

protected:
    MediaDescriptionHelio()
        /*: m_isVideo([track hasMediaCharacteristic:AVMediaCharacteristicVisual])
        , m_isAudio([track hasMediaCharacteristic:AVMediaCharacteristicAudible])
        , m_isText([track hasMediaCharacteristic:AVMediaCharacteristicLegible])*/
    {
        // NSArray* formatDescriptions = [track formatDescriptions];
        // CMFormatDescriptionRef description = [formatDescriptions count] ? (CMFormatDescriptionRef)[formatDescriptions objectAtIndex:0] : 0;
        // if (description) {
        //     FourCharCode codec = CMFormatDescriptionGetMediaSubType(description);
        //     m_codec = AtomicString(reinterpret_cast<LChar*>(&codec), 4);
        // }
    }

    AtomicString m_codec;
    bool m_isVideo;
    bool m_isAudio;
    bool m_isText;
};

// To callback from receiving an avpacket..

// TOOD: by default we want these callbacks to happen on the helio thread?
// Or on the main thread?


// if (m_client) {
//     Ref<MediaSample> mediaSample = MediaSampleAVFObjC::create(sampleBuffer, trackID);
//     LOG(MediaSourceSamples, "SourceBufferPrivateAVFObjC::processCodedFrame(%p) - sample(%s)", this, toString(mediaSample.get()).utf8().data());
//     m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);
// }

RefPtr<SourceBufferPrivateHelio> SourceBufferPrivateHelio::create(MediaSourcePrivateHelio* parent) {
    printf("SourceBufferPrivateHelio create\n");
    return adoptRef(new SourceBufferPrivateHelio(parent));
}

SourceBufferPrivateHelio::SourceBufferPrivateHelio(MediaSourcePrivateHelio* parent __attribute__((unused)))
  : m_readyState(MediaPlayer::HaveNothing) {
      printf("SourceBufferPrivateHelio constructor\n");
}

SourceBufferPrivateHelio::~SourceBufferPrivateHelio() {
    printf("SourceBufferPrivateHelio destructor\n");
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
void SourceBufferPrivateHelio::append(const unsigned char* data __attribute__((unused)), unsigned length) {
    printf("SourceBufferPrivateHelio append size:%i\n", length);
  // TODO: SourceBuffer has a ifdef GSTREAMER wrapper around quota exceeded error
  // buffer size is also defined in SourceBuffer, should be a private implementation

  // bool didSuccessfullyParse = true;
  // TODO: Don't call this until the helio engine reports being done with
  // demuxing the data..


  // TODO: You must first call
  // sourceBufferPrivateDidReceiveInitializationSegment
  // or you get an appendError

  /*

  struct InitializationSegment {
      MediaTime duration;

      struct AudioTrackInformation {
          RefPtr<MediaDescription> description;
          RefPtr<AudioTrackPrivate> track;
      };
      Vector<AudioTrackInformation> audioTracks;

      struct VideoTrackInformation {
          RefPtr<MediaDescription> description;
          RefPtr<VideoTrackPrivate> track;
      };
      Vector<VideoTrackInformation> videoTracks;

      struct TextTrackInformation {
          RefPtr<MediaDescription> description;
          RefPtr<InbandTextTrackPrivate> track;
      };
      Vector<TextTrackInformation> textTracks;
  };
  virtual void sourceBufferPrivateDidReceiveInitializationSegment(SourceBufferPrivate*, const InitializationSegment&) = 0;


  */

  static int once = 0;
  if (!once) {
      once++;
      SourceBufferPrivateClient::InitializationSegment segment;
      segment.duration = MediaTime(); //TODO:

      SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;
      vtInfo.track = VideoTrackPrivate::create(); // TODO:
      vtInfo.description = MediaDescriptionHelio::create(); // TODO:

      segment.videoTracks.append(vtInfo);

      SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;
      atInfo.track = AudioTrackPrivate::create(); // TODO:
      atInfo.description = MediaDescriptionHelio::create(); // TODO:

      segment.audioTracks.append(atInfo);

      SourceBufferPrivateClient::InitializationSegment::TextTrackInformation ttInfo;
      /*
      enum CueFormat {
          Data,
          Generic,
          WebVTT
      };
      */
      ttInfo.track = InbandTextTrackPrivate::create(InbandTextTrackPrivate::CueFormat::Generic); // TODO:
      ttInfo.description = MediaDescriptionHelio::create(); // TODO:

      segment.textTracks.append(ttInfo);

      m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);


      return;
  }


  if (m_client) {
      Ref<MediaSample> mediaSample = MediaSampleHelio::create();
      //LOG(MediaSourceSamples, "SourceBufferPrivateAVFObjC::processCodedFrame(%p) - sample(%s)", this, toString(mediaSample.get()).utf8().data());
      m_client->sourceBufferPrivateDidReceiveSample(this, mediaSample);
  }




  if (m_client) {
      m_client->sourceBufferPrivateAppendComplete(this, SourceBufferPrivateClient::AppendSucceeded);
  }
  // if (m_client)
  //     m_client->sourceBufferPrivateAppendComplete(this, didSuccessfullyParse ? SourceBufferPrivateClient::AppendSucceeded : SourceBufferPrivateClient::ParsingFailed);
}

void SourceBufferPrivateHelio::abort() {
    printf("SourceBufferPrivateHelio abort\n");
}

/*
1   com.apple.WebCore             	0x000000010c664cbd WebCore::SourceBufferPrivateHelio::resetParserState() + 45
2   com.apple.WebCore             	0x000000010c080d33 WebCore::SourceBuffer::resetParserState() + 435 (SourceBuffer.cpp:272)
3   com.apple.WebCore             	0x000000010c08523f WebCore::SourceBuffer::appendError(bool) + 95 (SourceBuffer.cpp:1401)
4   com.apple.WebCore             	0x000000010c08ab7b WebCore::SourceBuffer::sourceBufferPrivateDidReceiveSample(WebCore::SourceBufferPrivate*, WebCore::MediaSample&) + 123 (SourceBuffer.cpp:1427)
5   com.apple.WebCore             	0x000000010c08d0b4 non-virtual thunk to WebCore::SourceBuffer::sourceBufferPrivateDidReceiveSample(WebCore::SourceBufferPrivate*, WebCore::MediaSample&) + 52 (SourceBuffer.cpp:1414)
*/
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

}
