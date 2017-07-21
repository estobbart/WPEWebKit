/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RealtimeMediaSourceSupportedConstraints.h"

#if ENABLE(MEDIA_STREAM)

#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

const AtomicString& RealtimeMediaSourceSupportedConstraints::nameForConstraint(MediaConstraintType constraint)
{
    static NeverDestroyed<AtomicString> unknownConstraintName(emptyString());
    static NeverDestroyed<AtomicString> widthConstraintName("width");
    static NeverDestroyed<AtomicString> heightConstraintName("height");
    static NeverDestroyed<AtomicString> aspectRatioConstraintName("aspectRatio");
    static NeverDestroyed<AtomicString> frameRateConstraintName("frameRate");
    static NeverDestroyed<AtomicString> facingModeConstraintName("facingMode");
    static NeverDestroyed<AtomicString> volumeConstraintName("volume");
    static NeverDestroyed<AtomicString> sampleRateConstraintName("sampleRate");
    static NeverDestroyed<AtomicString> sampleSizeConstraintName("sampleSize");
    static NeverDestroyed<AtomicString> echoCancellationConstraintName("echoCancellation");
    static NeverDestroyed<AtomicString> deviceIdConstraintName("deviceId");
    static NeverDestroyed<AtomicString> groupIdConstraintName("groupId");

    static NeverDestroyed<AtomicString> minWidthConstraintName("minWidth");
    static NeverDestroyed<AtomicString> maxWidthConstraintName("maxWidth");
    static NeverDestroyed<AtomicString> minHeightConstraintName("minHeight");
    static NeverDestroyed<AtomicString> maxHeightConstraintName("maxHeight");
    static NeverDestroyed<AtomicString> minFrameRateConstraintName("minFrameRate");
    static NeverDestroyed<AtomicString> maxFrameRateConstraintName("maxFrameRate");
    static NeverDestroyed<AtomicString> minAspectRatioConstraintName("minAspectRatio");
    static NeverDestroyed<AtomicString> maxAspectRatioConstraintName("maxAspectRatio");
    static NeverDestroyed<AtomicString> googEchoCancellationConstraintName("googEchoCancellation");
    static NeverDestroyed<AtomicString> googEchoCancellation2ConstraintName("googEchoCancellation2");
    static NeverDestroyed<AtomicString> googDAEchoCancellationConstraintName("googDAEchoCancellation");
    static NeverDestroyed<AtomicString> googAutoGainControlConstraintName("googAutoGainControl");
    static NeverDestroyed<AtomicString> googAutoGainControl2ConstraintName("googAutoGainControl2");
    static NeverDestroyed<AtomicString> googNoiseSuppressionConstraintName("googNoiseSuppression");
    static NeverDestroyed<AtomicString> googNoiseSuppression2ConstraintName("googNoiseSuppression2");
    static NeverDestroyed<AtomicString> googHighpassFilterConstraintName("googHighpassFilter");
    static NeverDestroyed<AtomicString> googTypingNoiseDetectionConstraintName("googTypingNoiseDetection");
    static NeverDestroyed<AtomicString> googAudioMirroringConstraintName("googAudioMirroring");
    static NeverDestroyed<AtomicString> audioDebugRecordingConstraintName("audioDebugRecording");
    static NeverDestroyed<AtomicString> googNoiseReductionConstraintName("googNoiseReduction");
    static NeverDestroyed<AtomicString> offerToReceiveAudioConstraintName("offerToReceiveAudio");
    static NeverDestroyed<AtomicString> offerToReceiveVideoConstraintName("offerToReceiveVideo");
    static NeverDestroyed<AtomicString> voiceActivityDetectionConstraintName("VoiceActivityDetection");
    static NeverDestroyed<AtomicString> iceRestartConstraintName("IceRestart");
    static NeverDestroyed<AtomicString> googUseRtpMUXConstraintName("googUseRtpMUX");
    static NeverDestroyed<AtomicString> dtlsSrtpKeyAgreementConstraintName("DtlsSrtpKeyAgreement");
    static NeverDestroyed<AtomicString> rtpDataChannelsConstraintName("RtpDataChannels");
    static NeverDestroyed<AtomicString> preferh264ConstraintName("preferh264");
    static NeverDestroyed<AtomicString> ignoreInactiveInterfacesConstraintName("IgnoreInactiveInterfaces");
    static NeverDestroyed<AtomicString> googDscpConstraintName("googDscp");
    static NeverDestroyed<AtomicString> googIPv6ConstraintName("googIPv6");
    static NeverDestroyed<AtomicString> googSuspendBelowMinBitrateConstraintName("googSuspendBelowMinBitrate");
    static NeverDestroyed<AtomicString> googNumUnsignalledRecvStreamsConstraintName("googNumUnsignalledRecvStreams");
    static NeverDestroyed<AtomicString> googCombinedAudioVideoBweConstraintName("googCombinedAudioVideoBwe");
    static NeverDestroyed<AtomicString> googScreencastMinBitrateConstraintName("googScreencastMinBitrate");
    static NeverDestroyed<AtomicString> googCpuOveruseDetectionConstraintName("googCpuOveruseDetection");
    static NeverDestroyed<AtomicString> googCpuUnderuseThresholdConstraintName("googCpuUnderuseThreshold");
    static NeverDestroyed<AtomicString> googCpuOveruseThresholdConstraintName("googCpuOveruseThreshold");
    static NeverDestroyed<AtomicString> googCpuUnderuseEncodeRsdThresholdConstraintName("googCpuUnderuseEncodeRsdThreshold");
    static NeverDestroyed<AtomicString> googCpuOveruseEncodeRsdThresholdConstraintName("googCpuOveruseEncodeRsdThreshold");
    static NeverDestroyed<AtomicString> googCpuOveruseEncodeUsageConstraintName("googCpuOveruseEncodeUsage");
    static NeverDestroyed<AtomicString> googHighStartBitrateConstraintName("googHighStartBitrate");
    static NeverDestroyed<AtomicString> googHighBitrateConstraintName("googHighBitrate");
    static NeverDestroyed<AtomicString> googVeryHighBitrateConstraintName("googVeryHighBitrate");
    static NeverDestroyed<AtomicString> googPayloadPaddingConstraintName("googPayloadPadding");

    switch (constraint) {
    case MediaConstraintType::Unknown:
        return unknownConstraintName;
    case MediaConstraintType::Width:
        return widthConstraintName;
    case MediaConstraintType::Height:
        return heightConstraintName;
    case MediaConstraintType::AspectRatio:
        return aspectRatioConstraintName;
    case MediaConstraintType::FrameRate:
        return frameRateConstraintName;
    case MediaConstraintType::FacingMode:
        return facingModeConstraintName;
    case MediaConstraintType::Volume:
        return volumeConstraintName;
    case MediaConstraintType::SampleRate:
        return sampleRateConstraintName;
    case MediaConstraintType::SampleSize:
        return sampleSizeConstraintName;
    case MediaConstraintType::EchoCancellation:
        return echoCancellationConstraintName;
    case MediaConstraintType::DeviceId:
        return deviceIdConstraintName;
    case MediaConstraintType::GroupId:
        return groupIdConstraintName;

    // Legacy Constraints - for compatibility
    case MediaConstraintType::MinWidth:
        return minWidthConstraintName;
    case MediaConstraintType::MaxWidth:
        return maxWidthConstraintName;
    case MediaConstraintType::MinHeight:
        return minHeightConstraintName;
    case MediaConstraintType::MaxHeight:
        return maxHeightConstraintName;
    case MediaConstraintType::MinFrameRate:
        return minFrameRateConstraintName;
    case MediaConstraintType::MaxFrameRate:
        return maxFrameRateConstraintName;
    case MediaConstraintType::MinAspectRatio:
        return minAspectRatioConstraintName;
    case MediaConstraintType::MaxAspectRatio:
        return maxAspectRatioConstraintName;
    case MediaConstraintType::GoogEchoCancellation:
        return googEchoCancellationConstraintName;
    case MediaConstraintType::GoogEchoCancellation2:
        return googEchoCancellation2ConstraintName;
    case MediaConstraintType::GoogDAEchoCancellation:
        return googDAEchoCancellationConstraintName;
    case MediaConstraintType::GoogAutoGainControl:
        return googAutoGainControlConstraintName;
    case MediaConstraintType::GoogAutoGainControl2:
        return googAutoGainControl2ConstraintName;
    case MediaConstraintType::GoogNoiseSuppression:
        return googNoiseSuppressionConstraintName;
    case MediaConstraintType::GoogNoiseSuppression2:
        return googNoiseSuppression2ConstraintName;
    case MediaConstraintType::GoogHighpassFilter:
        return googHighpassFilterConstraintName;
    case MediaConstraintType::GoogTypingNoiseDetection:
        return googTypingNoiseDetectionConstraintName;
    case MediaConstraintType::GoogAudioMirroring:
        return googAudioMirroringConstraintName;
    case MediaConstraintType::AudioDebugRecording:
        return audioDebugRecordingConstraintName;
    case MediaConstraintType::GoogNoiseReduction:
        return googNoiseReductionConstraintName;
    case MediaConstraintType::OfferToReceiveAudio:
        return offerToReceiveAudioConstraintName;
    case MediaConstraintType::OfferToReceiveVideo:
        return offerToReceiveVideoConstraintName;
    case MediaConstraintType::VoiceActivityDetection:
        return voiceActivityDetectionConstraintName;
    case MediaConstraintType::IceRestart:
        return iceRestartConstraintName;
    case MediaConstraintType::GoogUseRtpMUX:
        return googUseRtpMUXConstraintName;
    case MediaConstraintType::DtlsSrtpKeyAgreement:
        return dtlsSrtpKeyAgreementConstraintName;
    case MediaConstraintType::RtpDataChannels:
        return rtpDataChannelsConstraintName;
    case MediaConstraintType::Preferh264:
        return preferh264ConstraintName;
    case MediaConstraintType::IgnoreInactiveInterfaces:
        return ignoreInactiveInterfacesConstraintName;
    case MediaConstraintType::GoogDscp:
        return googDscpConstraintName;
    case MediaConstraintType::GoogIPv6:
        return googIPv6ConstraintName;
    case MediaConstraintType::GoogSuspendBelowMinBitrate:
        return googSuspendBelowMinBitrateConstraintName;
    case MediaConstraintType::GoogNumUnsignalledRecvStreams:
        return googNumUnsignalledRecvStreamsConstraintName;
    case MediaConstraintType::GoogCombinedAudioVideoBwe:
        return googCombinedAudioVideoBweConstraintName;
    case MediaConstraintType::GoogScreencastMinBitrate:
        return googScreencastMinBitrateConstraintName;
    case MediaConstraintType::GoogCpuOveruseDetection:
        return googCpuOveruseDetectionConstraintName;
    case MediaConstraintType::GoogCpuUnderuseThreshold:
        return googCpuUnderuseThresholdConstraintName;
    case MediaConstraintType::GoogCpuOveruseThreshold:
        return googCpuOveruseThresholdConstraintName;
    case MediaConstraintType::GoogCpuUnderuseEncodeRsdThreshold:
        return googCpuUnderuseEncodeRsdThresholdConstraintName;
    case MediaConstraintType::GoogCpuOveruseEncodeRsdThreshold:
        return googCpuOveruseEncodeRsdThresholdConstraintName;
    case MediaConstraintType::GoogCpuOveruseEncodeUsage:
        return googCpuOveruseEncodeUsageConstraintName;
    case MediaConstraintType::GoogHighStartBitrate:
        return googHighStartBitrateConstraintName;
    case MediaConstraintType::GoogHighBitrate:
        return googHighBitrateConstraintName;
    case MediaConstraintType::GoogVeryHighBitrate:
        return googVeryHighBitrateConstraintName;
    case MediaConstraintType::GoogPayloadPadding:
        return googPayloadPaddingConstraintName;
    }

    ASSERT_NOT_REACHED();
    return emptyAtom;
}

MediaConstraintType RealtimeMediaSourceSupportedConstraints::constraintFromName(const String& constraintName)
{
    static NeverDestroyed<HashMap<AtomicString, MediaConstraintType>> nameToConstraintMap;
    HashMap<AtomicString, MediaConstraintType>& nameToConstraintMapValue = nameToConstraintMap.get();
    if (!nameToConstraintMapValue.size()) {
        nameToConstraintMapValue.add("width", MediaConstraintType::Width);
        nameToConstraintMapValue.add("height", MediaConstraintType::Height);
        nameToConstraintMapValue.add("aspectRatio", MediaConstraintType::AspectRatio);
        nameToConstraintMapValue.add("frameRate", MediaConstraintType::FrameRate);
        nameToConstraintMapValue.add("facingMode", MediaConstraintType::FacingMode);
        nameToConstraintMapValue.add("volume", MediaConstraintType::Volume);
        nameToConstraintMapValue.add("sampleRate", MediaConstraintType::SampleRate);
        nameToConstraintMapValue.add("sampleSize", MediaConstraintType::SampleSize);
        nameToConstraintMapValue.add("echoCancellation", MediaConstraintType::EchoCancellation);
        nameToConstraintMapValue.add("deviceId", MediaConstraintType::DeviceId);
        nameToConstraintMapValue.add("groupId", MediaConstraintType::GroupId);

        // Legacy Constraints - for compatibility
        nameToConstraintMapValue.add("minWidth", MediaConstraintType::MinWidth);
        nameToConstraintMapValue.add("maxWidth", MediaConstraintType::MaxWidth);
        nameToConstraintMapValue.add("minHeight", MediaConstraintType::MinHeight);
        nameToConstraintMapValue.add("maxHeight", MediaConstraintType::MaxHeight);
        nameToConstraintMapValue.add("minFrameRate", MediaConstraintType::MinFrameRate);
        nameToConstraintMapValue.add("maxFrameRate", MediaConstraintType::MaxFrameRate);
        nameToConstraintMapValue.add("minAspectRatio", MediaConstraintType::MinAspectRatio);
        nameToConstraintMapValue.add("maxAspectRatio", MediaConstraintType::MaxAspectRatio);
        nameToConstraintMapValue.add("googEchoCancellation", MediaConstraintType::GoogEchoCancellation);
        nameToConstraintMapValue.add("googEchoCancellation2", MediaConstraintType::GoogEchoCancellation2);
        nameToConstraintMapValue.add("googDAEchoCancellation", MediaConstraintType::GoogDAEchoCancellation);
        nameToConstraintMapValue.add("googAutoGainControl", MediaConstraintType::GoogAutoGainControl);
        nameToConstraintMapValue.add("googAutoGainControl2", MediaConstraintType::GoogAutoGainControl2);
        nameToConstraintMapValue.add("googNoiseSuppression", MediaConstraintType::GoogNoiseSuppression);
        nameToConstraintMapValue.add("googNoiseSuppression2", MediaConstraintType::GoogNoiseSuppression2);
        nameToConstraintMapValue.add("googHighpassFilter", MediaConstraintType::GoogHighpassFilter);
        nameToConstraintMapValue.add("googTypingNoiseDetection", MediaConstraintType::GoogTypingNoiseDetection);
        nameToConstraintMapValue.add("googAudioMirroring", MediaConstraintType::GoogAudioMirroring);
        nameToConstraintMapValue.add("audioDebugRecording", MediaConstraintType::AudioDebugRecording);
        nameToConstraintMapValue.add("googNoiseReduction", MediaConstraintType::GoogNoiseReduction);
        nameToConstraintMapValue.add("offerToReceiveAudio", MediaConstraintType::OfferToReceiveAudio);
        nameToConstraintMapValue.add("offerToReceiveVideo", MediaConstraintType::OfferToReceiveVideo);
        nameToConstraintMapValue.add("VoiceActivityDetection", MediaConstraintType::VoiceActivityDetection);
        nameToConstraintMapValue.add("IceRestart", MediaConstraintType::IceRestart);
        nameToConstraintMapValue.add("googUseRtpMUX", MediaConstraintType::GoogUseRtpMUX);
        nameToConstraintMapValue.add("DtlsSrtpKeyAgreement", MediaConstraintType::DtlsSrtpKeyAgreement);
        nameToConstraintMapValue.add("RtpDataChannels", MediaConstraintType::RtpDataChannels);
        nameToConstraintMapValue.add("preferh264", MediaConstraintType::Preferh264);
        nameToConstraintMapValue.add("IgnoreInactiveInterfaces", MediaConstraintType::IgnoreInactiveInterfaces);
        nameToConstraintMapValue.add("googDscp", MediaConstraintType::GoogDscp);
        nameToConstraintMapValue.add("googIPv6", MediaConstraintType::GoogIPv6);
        nameToConstraintMapValue.add("googSuspendBelowMinBitrate", MediaConstraintType::GoogSuspendBelowMinBitrate);
        nameToConstraintMapValue.add("googNumUnsignalledRecvStreams", MediaConstraintType::GoogNumUnsignalledRecvStreams);
        nameToConstraintMapValue.add("googCombinedAudioVideoBwe", MediaConstraintType::GoogCombinedAudioVideoBwe);
        nameToConstraintMapValue.add("googScreencastMinBitrate", MediaConstraintType::GoogScreencastMinBitrate);
        nameToConstraintMapValue.add("googCpuOveruseDetection", MediaConstraintType::GoogCpuOveruseDetection);
        nameToConstraintMapValue.add("googCpuUnderuseThreshold", MediaConstraintType::GoogCpuUnderuseThreshold);
        nameToConstraintMapValue.add("googCpuOveruseThreshold", MediaConstraintType::GoogCpuOveruseThreshold);
        nameToConstraintMapValue.add("googCpuUnderuseEncodeRsdThreshold", MediaConstraintType::GoogCpuUnderuseEncodeRsdThreshold);
        nameToConstraintMapValue.add("googCpuOveruseEncodeRsdThreshold", MediaConstraintType::GoogCpuOveruseEncodeRsdThreshold);
        nameToConstraintMapValue.add("googCpuOveruseEncodeUsage", MediaConstraintType::GoogCpuOveruseEncodeUsage);
        nameToConstraintMapValue.add("googHighStartBitrate", MediaConstraintType::GoogHighStartBitrate);
        nameToConstraintMapValue.add("googHighBitrate", MediaConstraintType::GoogHighBitrate);
        nameToConstraintMapValue.add("googVeryHighBitrate", MediaConstraintType::GoogVeryHighBitrate);
        nameToConstraintMapValue.add("googPayloadPadding", MediaConstraintType::GoogPayloadPadding);
    }
    auto iter = nameToConstraintMapValue.find(constraintName);
    return iter == nameToConstraintMapValue.end() ? MediaConstraintType::Unknown : iter->value;
}

bool RealtimeMediaSourceSupportedConstraints::supportsConstraint(MediaConstraintType constraint) const
{
    switch (constraint) {
    case MediaConstraintType::Unknown:
        return false;
    case MediaConstraintType::Width:
        return supportsWidth();
    case MediaConstraintType::Height:
        return supportsHeight();
    case MediaConstraintType::AspectRatio:
        return supportsAspectRatio();
    case MediaConstraintType::FrameRate:
        return supportsFrameRate();
    case MediaConstraintType::FacingMode:
        return supportsFacingMode();
    case MediaConstraintType::Volume:
        return supportsVolume();
    case MediaConstraintType::SampleRate:
        return supportsSampleRate();
    case MediaConstraintType::SampleSize:
        return supportsSampleSize();
    case MediaConstraintType::EchoCancellation:
        return supportsEchoCancellation();
    case MediaConstraintType::DeviceId:
        return supportsDeviceId();
    case MediaConstraintType::GroupId:
        return supportsGroupId();
    }

    ASSERT_NOT_REACHED();
    return false;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
