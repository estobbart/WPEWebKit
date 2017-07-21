/*
* Copyright (c) 2016, Comcast
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR OR; PROFITS BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY OF THEORY LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "RealtimeMediaSourceCenterQt5WebRTC.h"

#if ENABLE(MEDIA_STREAM)

#include "MediaStream.h"
#include "MediaStreamPrivate.h"
#include "MediaStreamTrack.h"
#include "CaptureDevice.h"
#include "UUID.h"

#include <wtf/NeverDestroyed.h>
#include <wtf/MainThread.h>
#include <NotImplemented.h>

#include "FloatRect.h"

#include <cairo.h>

namespace WebCore {

void enableQt5WebRTCPeerConnectionBackend();

WRTCInt::RTCMediaSourceCenter& getRTCMediaSourceCenter()
{
    static std::unique_ptr<WRTCInt::RTCMediaSourceCenter> rtcMediaSourceCenter;
    if (!rtcMediaSourceCenter)
        rtcMediaSourceCenter.reset(WRTCInt::createRTCMediaSourceCenter());
    return *rtcMediaSourceCenter.get();
}

void RealtimeMediaSourceQt5WebRTC::startProducingData()
{
    if (m_stream) {
        m_isProducingData = true;
    }
}

void RealtimeMediaSourceQt5WebRTC::stopProducingData()
{
    if (m_isProducingData) {
        m_isProducingData = false;
        m_stream.reset();
    }
}

RealtimeVideoSourceQt5WebRTC::RealtimeVideoSourceQt5WebRTC(const String& id, const String& name)
    : RealtimeMediaSourceQt5WebRTC(id, RealtimeMediaSource::Video, name)
{
    // TODO: obtain settings from the device
    m_currentSettings.setWidth(320);
    m_currentSettings.setHeight(240);
}

RealtimeMediaSourceCenter& RealtimeMediaSourceCenter::platformCenter()
{
    ASSERT(isMainThread());

    static NeverDestroyed<RealtimeMediaSourceCenterQt5WebRTC> center;
    return center;
}

RealtimeMediaSourceCenterQt5WebRTC::RealtimeMediaSourceCenterQt5WebRTC()
{
    WRTCInt::init();

    enableQt5WebRTCPeerConnectionBackend();

    m_supportedConstraints.setSupportsWidth(true);
    m_supportedConstraints.setSupportsHeight(true);
}

void RealtimeMediaSourceCenterQt5WebRTC::validateRequestConstraints(ValidConstraintsHandler validHandler, 
    InvalidConstraintsHandler, const MediaConstraints& audioConstraints, const MediaConstraints& videoConstraints)
{
    bool needsAudio = audioConstraints.isValid();
    bool needsVideo = videoConstraints.isValid();

    Vector<String> audioNames;
    Vector<String> videoNames;

    enumerateSources(needsAudio, needsVideo, audioNames, videoNames);

    Vector<String> audioSources;
    Vector<String> videoSources;

    for (int i = 0; i < audioNames.size(); ++i) {
        audioSources.append(createCanonicalUUIDString());
    }
    for (int i = 0; i < videoNames.size(); ++i) {
        videoSources.append(createCanonicalUUIDString());
    }

    validHandler(WTFMove(audioSources), WTFMove(videoSources));
}

void RealtimeMediaSourceCenterQt5WebRTC::createMediaStream(NewMediaStreamHandler handler, const String& audioDeviceID, const String& videoDeviceID, const MediaConstraints*, const MediaConstraints*)
{
    Vector<String> audioNames;
    Vector<String> videoNames;

    enumerateSources(!audioDeviceID.isEmpty(), !videoDeviceID.isEmpty(), audioNames, videoNames);

    RefPtr<RealtimeMediaSourceQt5WebRTC> audioSource = !audioNames.isEmpty() ? adoptRef(new RealtimeAudioSourceQt5WebRTC(audioDeviceID, audioNames[0])) : nullptr;
    RefPtr<RealtimeMediaSourceQt5WebRTC> videoSource = !videoNames.isEmpty() ? adoptRef(new RealtimeVideoSourceQt5WebRTC(videoDeviceID, videoNames[0])) : nullptr;

    String audioSourceName = audioSource ? audioSource->name() : String();
    String videoSourceName = videoSource ? videoSource->name() : String();

    std::shared_ptr<WRTCInt::RTCMediaStream> rtcStream(
        getRTCMediaSourceCenter().createMediaStream(
            audioSourceName.utf8().data(), videoSourceName.utf8().data()));

    Vector<Ref<RealtimeMediaSource>> audioSources;
    Vector<Ref<RealtimeMediaSource>> videoSources;

    if (audioSource) {
        audioSource->setRTCStream(rtcStream);
        audioSources.append(audioSource.releaseNonNull());
    }

    if (videoSource) {
        videoSource->setRTCStream(rtcStream);
        videoSources.append(videoSource.releaseNonNull());
    }

    String id = rtcStream->id().c_str();

    handler(MediaStreamPrivate::create(id, audioSources, videoSources));
}


void RealtimeMediaSourceCenterQt5WebRTC::enumerateSources(bool needsAudio, bool needsVideo, Vector<String>& audios, Vector<String>& videos)
{
    if (needsAudio) {
        std::vector<std::string> audioDevices;
        WRTCInt::enumerateDevices(WRTCInt::AUDIO, audioDevices);
        for (auto& device : audioDevices) {
            audios.append(device.c_str());
        }
    }

    if (needsVideo) {
        std::vector<std::string> videoDevices;
        WRTCInt::enumerateDevices(WRTCInt::VIDEO, videoDevices);
        for (auto& device : videoDevices) {
            videos.append(device.c_str());
        }
    }
}

Vector<CaptureDevice> RealtimeMediaSourceCenterQt5WebRTC::getMediaStreamDevices()
{
    return Vector<CaptureDevice>();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
