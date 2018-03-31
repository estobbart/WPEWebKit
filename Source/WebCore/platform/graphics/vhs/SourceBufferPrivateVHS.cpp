
#include "config.h"
#include "SourceBufferPrivateVHS.h"

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include <cmath>
#include <stdio.h>
#include "SourceBufferPrivateClient.h"
#include "AudioTrackPrivateVHS.h"
#include "VideoTrackPrivateVHS.h"
#include "DataLoaderProxy.h"
#include "MediaSampleVHS.h"
#include "InitializationSegmentVHS.h"
#include "MediaSegmentVHS.h"

namespace WebCore {

// void hex_dump_buffer(uint8_t *buffer, size_t size) {
//     uint8_t count = 0;
//     while (size) {
//         printf(" %02x", buffer[0]);
//         size--;
//         buffer++;
//         count++;
//         if (count == 16) {
//             printf("\n");
//             count = 0;
//         }
//     }
//     if (count) {
//         printf("\n");
//     }
// }

// This callback happens from all sorts of places. fix pending in libvhs.
// TODO: in the meantime a single shot event needs to fire to support this.
static void rcv_media_read_callback_source_buffer(void *context) {
    printf("SourceBufferPrivateVHS %s \n", __PRETTY_FUNCTION__);
    SourceBufferPrivateVHS *sourceBuffer = (SourceBufferPrivateVHS *)context;
    sourceBuffer->streamBufferAvailable();
}

RefPtr<SourceBufferPrivateVHS> SourceBufferPrivateVHS::create(const String& codec, Ref<DataLoaderProxy>&& proxy, Ref<MediaPlatformPrivate>&& platform) {
    return adoptRef(new SourceBufferPrivateVHS(codec, WTFMove(proxy), WTFMove(platform)));
}

SourceBufferPrivateVHS::SourceBufferPrivateVHS(const String& codec, Ref<DataLoaderProxy>&& proxy, Ref<MediaPlatformPrivate>&& platform)
    : m_dataLoaderProxy(WTFMove(proxy))
    , m_mediaPlatformPrivate(WTFMove(platform))
    , m_state(MediaPlayer::HaveNothing)
    , m_currentSample(nullptr)
    , m_flushed(false)
    , m_appending(false)
    , m_notifyWhenReadyForSamples(false)
    , m_bufferAvailable(false) {
    printf("SourceBufferPrivateVHS constructor: %s\n", codec.utf8().data());
    m_vhsPipeline = NULL;
    // TODO: Fix the remove SourceBuffer
    m_vhsParser = vhs_parser_init(NULL, NULL);

    Vector<String> codecParts = codec.split(".");

    m_vhsStream =  vhs_media_stream_init(codecParts.first().utf8().data(),
                                         rcv_media_read_callback_source_buffer,
                                         this);
}

SourceBufferPrivateVHS::~SourceBufferPrivateVHS() {
    printf("!!!!!!!!    SourceBufferPrivateVHS destructor    !!!!!!!!\n");
    if (m_vhsParser) {
        vhs_parser_destroy(&m_vhsParser);
    }
}


void SourceBufferPrivateVHS::setClient(SourceBufferPrivateClient* client) {
    printf("%s\n", __PRETTY_FUNCTION__);
    m_client = client;
    // TODO: is this better in setClient or removedFromMediaSource
    if (!m_client) {
        m_dataLoaderProxy->removeSourceBuffer(this);
    }
}

// Temporary debug function..
// void dump_node_names(vhs_node_t *root) {
//     static uint8_t depth = 0;
//     vhs_cursor_t *cursor = vhs_cursor_init();
//
//     depth++;
//     char* indent = (char*)malloc(sizeof(char) * depth);
//     memset(indent, '-', sizeof(char) * depth);
//     while (cursor) {
//         vhs_node_t *child = vhs_node_next_child(root, &cursor);
//         if (child == NULL) {
//             break;
//         }
//
//         printf("%s %s\n", indent, vhs_node_type(child));
//
//         dump_node_names(child);
//     }
//     free(indent);
//     depth--;
// }

// https://w3c.github.io/media-source/isobmff-byte-stream-format.html
void SourceBufferPrivateVHS::append(const unsigned char* data,
                                    unsigned length) {
    printf("%s vhs_parse_isobmff start\n", __PRETTY_FUNCTION__);
    if (!m_client) {
        printf("Error No active client\n");
        return;
    }
    m_appending = true;
    bool error = false;
    // hex_dump_buffer(const_cast<unsigned char*>(data), length);
    vhs_node_t *root = vhs_node_init();
    vhs_parse_isobmff(m_vhsParser, root, const_cast<unsigned char*>(data), length);
    printf("%s vhs_parse_isobmff finished\n", __PRETTY_FUNCTION__);
    Ref<MediaSegmentVHS> vhsSegment = MediaSegmentVHS::create(root);

    // ISOBMFF Byte Stream Format Spec:
    // 3. Initialization Segments
    // An ISO BMFF initialization segment is defined in this specification as a
    // single File Type Box (ftyp) followed by a single Movie Box (moov).
    // 4. Media Segments
    // An ISO BMFF media segment is defined in this specification as one
    // optional Segment Type Box (styp) followed by a single Movie Fragment Box
    // (moof) followed by one or more Media Data Boxes (mdat). If the Segment
    // Type Box is not present, the segment must conform to the brands listed
    // in the File Type Box (ftyp) in the initialization segment.
    if (vhs_node_child(root, "ftyp") && vhs_node_child(root, "moov")) {
        m_initSegmentVHS = InitializationSegmentVHS::create(vhsSegment.copyRef());

        SourceBufferPrivateClient::InitializationSegment segment;

        if (m_initSegmentVHS->duration() == ULLONG_MAX) {
            segment.duration = MediaTime::positiveInfiniteTime();
        } else {
            segment.duration = MediaTime(m_initSegmentVHS->duration(), m_initSegmentVHS->timescale());
        }

        // TODO: The segment may have multiple tracks, trak boxes
        vhs_node_t *box = vhs_node_child(root, "trak");
        RefPtr<MediaDescriptionVHS> trackDescription = box ? createTrackDescription(box) : nullptr;

        if (!trackDescription) {
            printf("ERROR InitializationSegment did not provide codec\n");
            error = true;
        }

        box = vhs_node_child(root, "hdlr");
        vhs_hdlr_box_t *hdlr = VHS_HDLR_BOX(vhs_node_raw(box));
        if (hdlr) {

            char *format = vhs_hdlr_handler_type(hdlr);

            if (strcmp(format, "vide") == 0) {
                SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation vtInfo;

                vtInfo.track = VideoTrackPrivateVHS::create(m_initSegmentVHS->trackId());
                vtInfo.description = trackDescription;

                segment.videoTracks.append(vtInfo);

            } else if (strcmp(format, "soun") == 0) {
                SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation atInfo;

                atInfo.track = AudioTrackPrivateVHS::create(m_initSegmentVHS->trackId());
                atInfo.description = trackDescription;

                segment.audioTracks.append(atInfo);
            } else {
                printf("ERROR InitializationSegment did not provide hdlr type\n");
            }
        }


        if (!error) {
            if (m_state == MediaPlayer::HaveNothing) {
                m_state = MediaPlayer::HaveMetadata;
                m_dataLoaderProxy->sourceBufferDidTransition();
            }
            m_client->sourceBufferPrivateDidReceiveInitializationSegment(segment);
        }

    }
    // MSE identifies initialization segment infront of the media segment as a
    // contiguous sequence of segments
    if (vhs_node_child(root, "moof") && vhs_node_child(root, "mdat")) {
        vhs_node_t *node = vhsSegment->split();
        while (node) {
            // In the scenario where the content is missing full boxes, that
            // content should be disgarded.
            if (vhs_node_child(node, "moof") == NULL || vhs_node_child(node, "mdat") == NULL) {
                printf("Error unable to create a full MediaSample!\n");
                // dump_node_names(node);
                vhs_destroy_tree(&node);
                node = vhsSegment->split();
                continue;
            }
            printf("Splitting fragment -- \n");
            Ref<MediaSegmentVHS> partialSegment = MediaSegmentVHS::create(node);
            Ref<MediaSample> sample = MediaSampleVHS::create(WTFMove(partialSegment), *m_initSegmentVHS);
            m_client->sourceBufferPrivateDidReceiveSample(sample);
            node = vhsSegment->split();
        }
        // printf("Reporting last (or only) sample -- \n");
        // dump_node_names(vhsSegment->root());
        Ref<MediaSample> mediaSample = MediaSampleVHS::create(WTFMove(vhsSegment), *m_initSegmentVHS);
        m_client->sourceBufferPrivateDidReceiveSample(mediaSample);
    // } else {
        // TODO: Need to be able to determine if we didn't get either init
        // or media.
        // error = true;
        // printf("ERROR did not detect Initialization or Media Segment\n");
        // vhs_destroy_tree(&root);
    }

    // Finished parsing & constructing objects, report results...
    // TODO: what's the difference between ReadStreamFailed & ParsingFailed
    SourceBufferPrivateClient::AppendResult result = error ? SourceBufferPrivateClient::ParsingFailed : SourceBufferPrivateClient::AppendSucceeded;
    m_client->sourceBufferPrivateAppendComplete(result);
    if (!error) {
        m_dataLoaderProxy->sourceBufferDidAppendData();
    }
    m_appending = false;
}

RefPtr<MediaDescriptionVHS> SourceBufferPrivateVHS::createTrackDescription(vhs_node_t *tkhdNode) const {
    RefPtr<MediaDescriptionVHS> trackDescription = nullptr;

    vhs_node_t *box = vhs_node_child(tkhdNode, "stsd");
    if (box) {
        box = vhs_node_first_child(box);
        if (box) {
            char *type = vhs_node_type(box);
            // TODO: better way to determine codec...
            const char *codec = strcmp(type, "avcC") == 0 ? "avc1" : (strcmp(type, "esds") == 0 ? "mp4a" : type);
            trackDescription = MediaDescriptionVHS::create(codec);
        }
    }

    return trackDescription;
}

bool SourceBufferPrivateVHS::clientHasAudio() const {
    return m_client && m_client->sourceBufferPrivateHasAudio();
}

bool SourceBufferPrivateVHS::clientHasVideo() const {
    return m_client && m_client->sourceBufferPrivateHasVideo();
}

/**
 * TODO: What's actually being aborted here? Could use a test case.
 * The parsing/reporting of segments happens synchronously in the append call.
 */
void SourceBufferPrivateVHS::abort() {
    printf("%s\n", __PRETTY_FUNCTION__);
}

/**
 * Parser (vhs_parser_t m_vhsParser) doesn't actually keep any state.
 */
void SourceBufferPrivateVHS::resetParserState() {
    printf("%s\n", __PRETTY_FUNCTION__);
}

/**
 * removedFromMediaSource should free all the underlying resources
 * SourceBuffer & SourceBufferPrivate do not get destroyed until the garbage
 * collector cleans up the JSSourceBuffer.
 */
void SourceBufferPrivateVHS::removedFromMediaSource() {
    printf("%s\n", __PRETTY_FUNCTION__);
    // TODO: This might be better to do in the setActive false method
    m_notifyTaskQueue.close();
}

/**
 * readyState is only requested by the SourceBuffer and is used to determine
 * if it needs to upgrade/downgrade the state to MediaPlayer::HaveMetadata.
 * The only two states that can be checked here are HaveNothing & HaveMetadata.
 *
 * See setReadyState for additional details.
 */
MediaPlayer::ReadyState SourceBufferPrivateVHS::readyState() const {
    // printf("%s %p\n", __PRETTY_FUNCTION__, this);
    return m_state;
}

/**
 * A SourceBuffer will only call setReadyState with MediaPlayer::HaveMetadata
 * and only in certain scenarios where it checks the readyState first.
 *
 * It will upgrade the readyState once all buffers receive their initialization
 * segment, but only call the method on the last buffer to report an init segment.
 * Because it is only set on one buffer, SourceBufferPrivateVHS handles that
 * that state change and notification through the DataLoaderProxy during the
 * append cycle.
 *
 * It is also possible that the state gets downgraded due to removing frames.
 * TODO: still unclear why this would occur.
 */
void SourceBufferPrivateVHS::setReadyState(MediaPlayer::ReadyState state) {
    printf("%s %p %i\n", __PRETTY_FUNCTION__, this, state);
    if (state != MediaPlayer::HaveMetadata) {
        printf("ERROR: SourceBufferPrivateVHS(%p) Unexpected state change to: %i\n", this, state);
        return;
    }
    // TODO: This is needed before setActive, but it happens after..
    // if (state == MediaPlayer::HaveMetadata && !m_vhsPipeline) {
    //     m_vhsPipeline = vhs_media_pipeline_init(m_vhsStream);
    // }
    m_state = state;
}

/**
 * A SourceBuffer first becomes active after it reports it's first
 * initialization segment.
 */
void SourceBufferPrivateVHS::setActive(bool) {
    printf("%s %p\n", __PRETTY_FUNCTION__, this);
    if (!m_vhsPipeline) {
        m_vhsPipeline = vhs_media_pipeline_init(m_vhsStream);
    }
    printf("vhs_media_pipeline_init %p\n", m_vhsPipeline);
    m_mediaPlatformPrivate->connectPipeline(this);
}

/**
 * When flush gets called it should trigger the underlying engine to set the
 * start time to be the begining of the next sample it receives.
 *
 * TODO: support actually flushing any pending samples that may be in the decode
 * render pipeline.
 */
void SourceBufferPrivateVHS::flush(const AtomicString&) {
    printf("%s\n", __PRETTY_FUNCTION__);
    m_flushed = true;
    if (m_currentSample) {
        m_currentSample = nullptr;
        // Haven't seen this condition occur, but logging it in case.
        if (m_notifyWhenReadyForSamples) {
            printf("ERROR: m_notifyWhenReadyForSamples flag set in flush!\n");
        }
    }
}

/**
 * enqueueing a sample and writing a sample is two seperate actions
 * SourceBufferPrivateVHS always tries to keep a sample enqueued, it will only
 * write the sample when thheir is buffer space available in the vhs_stream.
 */
void SourceBufferPrivateVHS::enqueueSample(Ref<MediaSample>&& sample, const AtomicString& trackId) {
    printf("%s\n", __PRETTY_FUNCTION__);
    // TODO: support use of `sample->platformSample()``
    RefPtr<MediaSample> tmp = WTFMove(sample);
    m_currentSample = static_pointer_cast<MediaSampleVHS>(tmp);

    if (m_flushed) {
        m_flushed = false;
        // TODO: fix this and make libvhs actually notify the source buffer
        m_bufferAvailable = true;
        // TODO: use the media time base..
        uint64_t pts = round(tmp->decodeTime().toDouble() * 90000);
        vhs_media_stream_start_pts(m_vhsStream, pts);
    }

    // It is to be assumed that if a sample has been enqueued, and a buffer
    // available callback has occurred, that the sample should be written.
    // If a sample should not write, then one of those two things must not occur.
    // Preferrably, the buffer being available, since that is the layer that
    // this implementation has the most control over.
    if (m_bufferAvailable) {
        writeSampleToStream(trackId);
    }

}

void SourceBufferPrivateVHS::streamBufferAvailable() {
    printf("%s\n", __PRETTY_FUNCTION__);
    m_bufferAvailable = true;
    if (m_currentSample) {
        writeSampleToStream(AtomicString::number(m_initSegmentVHS->trackId()));
    }
}

/**
 * trackId is used only for notification puposes, if the entire sample is
 * consumed, and SourceBuffer needs to notify the client that it's ready for
 * another sample.
 */
void SourceBufferPrivateVHS::writeSampleToStream(const AtomicString& trackId) {
    printf("%s\n", __PRETTY_FUNCTION__);

    // TODO: check to see if the start time has been set or not (or if flush
    // has been called)
    uint8_t *buffer = NULL;
    size_t bufferSize = 0;
    vhs_media_stream_buffer(m_vhsStream, &buffer, &bufferSize);

    // TODO: This may not want to necessarily fill the buffer, and have an early
    // breakout for say 1s of data and let the callback signal to fill more.
    // This will better handle larger samples, or excessivly large platform
    // buffer sizes.
    // This could be done by checking the PES header for the timestamp
    // and keeping a running total.

    uint32_t nextPacketSize = m_currentSample->sizeOfNextPESPacket();
    size_t writeSize = 0;
    while (nextPacketSize && bufferSize >= nextPacketSize) {
        m_currentSample->writeNextPESPacket(buffer, &writeSize);

        // Confirm that the entire packet has been written
        if (writeSize != nextPacketSize) {
            printf("Error writeSize:%zu nextPacketSize:%zu\n", writeSize, nextPacketSize);
            break;
        }

        // NOTE(estobbart): Each PES packet gets written to the stream buffer
        // call into vhs to consume the buffer that's been written to.
        vhs_media_stream_write(m_vhsStream, writeSize);

        // NOTE(estobbart): PES packet headers can not be written to in the
        // middle of the buffer. They must be at the start of a buffer.
        // Reassign the buffer pointer and capture the available size.
        vhs_media_stream_buffer(m_vhsStream, &buffer, &bufferSize);

        // assign the size of the next packet for the loop condition
        nextPacketSize = m_currentSample->sizeOfNextPESPacket();
    }

    // If the vhs_Stream has run out of space in the buffer, but there are still
    // packets to write return early so the sb doesn't lose it's reference to
    // the current sample, or signal that it needs another sample.
    if (nextPacketSize) {
        m_bufferAvailable = false;
        if (bufferSize) {
            // NOTE(estobbart): write of 0 causes libvhs to query if space is
            // available, if it is, then that space is skipped. Once space
            // becomes available again the callback will fire and it can write
            // the next packet.
            vhs_media_stream_write(m_vhsStream, 0);
        }
        return;
    }

    // At this point the entire sample has been written to the stream buffer;
    // lose the sample reference, and notify that we're ready for more samples
    m_currentSample = nullptr;
    if (m_notifyWhenReadyForSamples) {
        m_notifyWhenReadyForSamples = false;
        m_notifyTaskQueue.enqueueTask(std::bind(&SourceBufferPrivateVHS::notifyClientTask, this, trackId));
    }
}

/**
 * NOTE(estobbart): The only two conditions here are if the SourceBuffer is
 * currently appending and if we haven't already enqueued a sample.
 * Determining if there is space available is a seperate operation.
 */
bool SourceBufferPrivateVHS::isReadyForMoreSamples(const AtomicString&) {
    printf("%s %p %i\n", __PRETTY_FUNCTION__, this, !m_appending && !m_currentSample);
    return !m_appending && !m_currentSample;
}

void SourceBufferPrivateVHS::notifyClientWhenReadyForMoreSamples(const AtomicString& trackId) {
    printf("%s %p\n", __PRETTY_FUNCTION__, this);
    if (m_appending && !m_currentSample) {
        if (!m_notifyTaskQueue.hasPendingTasks()) {
            m_notifyTaskQueue.enqueueTask(std::bind(&SourceBufferPrivateVHS::notifyClientTask, this, trackId));
        }
        return;
    }
    m_notifyWhenReadyForSamples = true;
}

void SourceBufferPrivateVHS::notifyClientTask(const AtomicString& trackId) {
    printf("%s %p\n", __PRETTY_FUNCTION__, this);
    m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples(trackId);
}

}

#endif
