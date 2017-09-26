/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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
#include "MediaConstraints.h"

#if ENABLE(MEDIA_STREAM)
#include "RealtimeMediaSourceCenter.h"
#include "RealtimeMediaSourceSupportedConstraints.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

const String& StringConstraint::find(std::function<bool(const String&)> filter) const
{
    for (auto& constraint : m_exact) {
        if (filter(constraint))
            return constraint;
    }

    for (auto& constraint : m_ideal) {
        if (filter(constraint))
            return constraint;
    }
    
    return emptyString();
}

double StringConstraint::fitnessDistance(const String& value) const
{
    // https://w3c.github.io/mediacapture-main/#dfn-applyconstraints

    // 1. If the constraint is not supported by the browser, the fitness distance is 0.
    if (isEmpty())
        return 0;

    // 2. If the constraint is required ('min', 'max', or 'exact'), and the settings
    //    dictionary's value for the constraint does not satisfy the constraint, the
    //    fitness distance is positive infinity.
    if (!m_exact.isEmpty() && m_exact.find(value) == notFound)
        return std::numeric_limits<double>::infinity();

    // 3. If no ideal value is specified, the fitness distance is 0.
    if (m_exact.isEmpty())
        return 0;

    // 5. For all string and enum non-required constraints (deviceId, groupId, facingMode,
    // echoCancellation), the fitness distance is the result of the formula
    //        (actual == ideal) ? 0 : 1
    return m_ideal.find(value) != notFound ? 0 : 1;
}

double StringConstraint::fitnessDistance(const Vector<String>& values) const
{
    if (isEmpty())
        return 0;

    double minimumDistance = std::numeric_limits<double>::infinity();
    for (auto& value : values)
        minimumDistance = std::min(minimumDistance, fitnessDistance(value));

    return minimumDistance;
}

void StringConstraint::merge(const MediaConstraint& other)
{
    ASSERT(other.isString());
    const StringConstraint& typedOther = downcast<StringConstraint>(other);

    if (typedOther.isEmpty())
        return;

    Vector<String> values;
    if (typedOther.getExact(values)) {
        if (m_exact.isEmpty())
            m_exact = values;
        else {
            for (auto& value : values) {
                if (m_exact.find(value) == notFound)
                    m_exact.append(value);
            }
        }
    }

    if (typedOther.getIdeal(values)) {
        if (m_ideal.isEmpty())
            m_ideal = values;
        else {
            for (auto& value : values) {
                if (m_ideal.find(value) == notFound)
                    m_ideal.append(value);
            }
        }
    }
}

void FlattenedConstraint::set(const MediaConstraint& constraint)
{
    for (auto& variant : m_variants) {
        if (variant.constraintType() == constraint.constraintType())
            return;
    }

    append(constraint);
}

void FlattenedConstraint::merge(const MediaConstraint& constraint)
{
    for (auto& variant : *this) {
        if (variant.constraintType() != constraint.constraintType())
            continue;

        switch (variant.dataType()) {
        case MediaConstraint::DataType::Integer:
            ASSERT(constraint.isInt());
            downcast<const IntConstraint>(variant).merge(downcast<const IntConstraint>(constraint));
            return;
        case MediaConstraint::DataType::Double:
            ASSERT(constraint.isDouble());
            downcast<const DoubleConstraint>(variant).merge(downcast<const DoubleConstraint>(constraint));
            return;
        case MediaConstraint::DataType::Boolean:
            ASSERT(constraint.isBoolean());
            downcast<const BooleanConstraint>(variant).merge(downcast<const BooleanConstraint>(constraint));
            return;
        case MediaConstraint::DataType::String:
            ASSERT(constraint.isString());
            downcast<const StringConstraint>(variant).merge(downcast<const StringConstraint>(constraint));
            return;
        case MediaConstraint::DataType::None:
            ASSERT_NOT_REACHED();
            return;
        }
    }

    append(constraint);
}

void FlattenedConstraint::append(const MediaConstraint& constraint)
{
#ifndef NDEBUG
    ++m_generation;
#endif

    m_variants.append(ConstraintHolder::create(constraint));
}

const MediaConstraint* FlattenedConstraint::find(MediaConstraintType type) const
{
    for (auto& variant : m_variants) {
        if (variant.constraintType() == type)
            return &variant.constraint();
    }

    return nullptr;
}

void MediaTrackConstraintSetMap::forEach(std::function<void(const MediaConstraint&)> callback) const
{
    filter([callback] (const MediaConstraint& constraint) mutable {
        callback(constraint);
        return false;
    });
}

void MediaTrackConstraintSetMap::filter(std::function<bool(const MediaConstraint&)> callback) const
{
    if (m_width && !m_width->isEmpty() && callback(*m_width))
        return;
    if (m_height && !m_height->isEmpty() && callback(*m_height))
        return;
    if (m_sampleRate && !m_sampleRate->isEmpty() && callback(*m_sampleRate))
        return;
    if (m_sampleSize && !m_sampleSize->isEmpty() && callback(*m_sampleSize))
        return;

    if (m_aspectRatio && !m_aspectRatio->isEmpty() && callback(*m_aspectRatio))
        return;
    if (m_frameRate && !m_frameRate->isEmpty() && callback(*m_frameRate))
        return;
    if (m_volume && !m_volume->isEmpty() && callback(*m_volume))
        return;

    if (m_echoCancellation && !m_echoCancellation->isEmpty() && callback(*m_echoCancellation))
        return;

    if (m_facingMode && !m_facingMode->isEmpty() && callback(*m_facingMode))
        return;
    if (m_deviceId && !m_deviceId->isEmpty() && callback(*m_deviceId))
        return;
    if (m_groupId && !m_groupId->isEmpty() && callback(*m_groupId))
        return;

    // Legacy Constraints - for compatibility
    if (m_minAspectRatio && !m_minAspectRatio->isEmpty() && callback(*m_minAspectRatio))
        return;
    if (m_maxAspectRatio && !m_maxAspectRatio->isEmpty() && callback(*m_maxAspectRatio))
        return;
    if (m_maxWidth && !m_maxWidth->isEmpty() && callback(*m_maxWidth))
        return;
    if (m_minWidth && !m_minWidth->isEmpty() && callback(*m_minWidth))
        return;
    if (m_maxHeight && !m_maxHeight->isEmpty() && callback(*m_maxHeight))
        return;
    if (m_minHeight && !m_minHeight->isEmpty() && callback(*m_minHeight))
        return;
    if (m_maxFrameRate && !m_maxFrameRate->isEmpty() && callback(*m_maxFrameRate))
        return;
    if (m_minFrameRate && !m_minFrameRate->isEmpty() && callback(*m_minFrameRate))
        return;
    if (m_googEchoCancellation && !m_googEchoCancellation->isEmpty() && callback(*m_googEchoCancellation))
        return;
    if (m_googEchoCancellation2 && !m_googEchoCancellation2->isEmpty() && callback(*m_googEchoCancellation2))
        return;
    if (m_googDAEchoCancellation && !m_googDAEchoCancellation->isEmpty() && callback(*m_googDAEchoCancellation))
        return;
    if (m_googAutoGainControl && !m_googAutoGainControl->isEmpty() && callback(*m_googAutoGainControl))
        return;
    if (m_googAutoGainControl2 && !m_googAutoGainControl2->isEmpty() && callback(*m_googAutoGainControl2))
        return;
    if (m_googNoiseSuppression && !m_googNoiseSuppression->isEmpty() && callback(*m_googNoiseSuppression))
        return;
    if (m_googNoiseSuppression2 && !m_googNoiseSuppression2->isEmpty() && callback(*m_googNoiseSuppression2))
        return;
    if (m_googHighpassFilter && !m_googHighpassFilter->isEmpty() && callback(*m_googHighpassFilter))
        return;
    if (m_googTypingNoiseDetection && !m_googTypingNoiseDetection->isEmpty() && callback(*m_googTypingNoiseDetection))
        return;
    if (m_googAudioMirroring && !m_googAudioMirroring->isEmpty() && callback(*m_googAudioMirroring))
        return;
    if (m_audioDebugRecording && !m_audioDebugRecording->isEmpty() && callback(*m_audioDebugRecording))
        return;
    if (m_googNoiseReduction && !m_googNoiseReduction->isEmpty() && callback(*m_googNoiseReduction))
        return;
    if (m_offerToReceiveAudio && !m_offerToReceiveAudio->isEmpty() && callback(*m_offerToReceiveAudio))
        return;
    if (m_offerToReceiveVideo && !m_offerToReceiveVideo->isEmpty() && callback(*m_offerToReceiveVideo))
        return;
    if (m_voiceActivityDetection && !m_voiceActivityDetection->isEmpty() && callback(*m_voiceActivityDetection))
        return;
    if (m_iceRestart && !m_iceRestart->isEmpty() && callback(*m_iceRestart))
        return;
    if (m_googUseRtpMUX && !m_googUseRtpMUX->isEmpty() && callback(*m_googUseRtpMUX))
        return;
    if (m_dtlsSrtpKeyAgreement && !m_dtlsSrtpKeyAgreement->isEmpty() && callback(*m_dtlsSrtpKeyAgreement))
        return;
    if (m_rtpDataChannels && !m_rtpDataChannels->isEmpty() && callback(*m_rtpDataChannels))
        return;
    if (m_preferh264 && !m_preferh264->isEmpty() && callback(*m_preferh264))
        return;
    if (m_ignoreInactiveInterfaces && !m_ignoreInactiveInterfaces->isEmpty() && callback(*m_ignoreInactiveInterfaces))
        return;
    if (m_googDscp && !m_googDscp->isEmpty() && callback(*m_googDscp))
        return;
    if (m_googIPv6 && !m_googIPv6->isEmpty() && callback(*m_googIPv6))
        return;
    if (m_googSuspendBelowMinBitrate && !m_googSuspendBelowMinBitrate->isEmpty() && callback(*m_googSuspendBelowMinBitrate))
        return;
    if (m_googNumUnsignalledRecvStreams && !m_googNumUnsignalledRecvStreams->isEmpty() && callback(*m_googNumUnsignalledRecvStreams))
        return;
    if (m_googCombinedAudioVideoBwe && !m_googCombinedAudioVideoBwe->isEmpty() && callback(*m_googCombinedAudioVideoBwe))
        return;
    if (m_googScreencastMinBitrate && !m_googScreencastMinBitrate->isEmpty() && callback(*m_googScreencastMinBitrate))
        return;
    if (m_googCpuOveruseDetection && !m_googCpuOveruseDetection->isEmpty() && callback(*m_googCpuOveruseDetection))
        return;
    if (m_googCpuUnderuseThreshold && !m_googCpuUnderuseThreshold->isEmpty() && callback(*m_googCpuUnderuseThreshold))
        return;
    if (m_googCpuOveruseThreshold && !m_googCpuOveruseThreshold->isEmpty() && callback(*m_googCpuOveruseThreshold))
        return;
    if (m_googCpuUnderuseEncodeRsdThreshold && !m_googCpuUnderuseEncodeRsdThreshold->isEmpty() && callback(*m_googCpuUnderuseEncodeRsdThreshold))
        return;
    if (m_googCpuOveruseEncodeRsdThreshold && !m_googCpuOveruseEncodeRsdThreshold->isEmpty() && callback(*m_googCpuOveruseEncodeRsdThreshold))
        return;
    if (m_googCpuOveruseEncodeUsage && !m_googCpuOveruseEncodeUsage->isEmpty() && callback(*m_googCpuOveruseEncodeUsage))
        return;
    if (m_googHighStartBitrate && !m_googHighStartBitrate->isEmpty() && callback(*m_googHighStartBitrate))
        return;
    if (m_googHighBitrate && !m_googHighBitrate->isEmpty() && callback(*m_googHighBitrate))
        return;
    if (m_googVeryHighBitrate && !m_googVeryHighBitrate->isEmpty() && callback(*m_googVeryHighBitrate))
        return;
    if (m_googPayloadPadding && !m_googPayloadPadding->isEmpty() && callback(*m_googPayloadPadding))
        return;
}

void MediaTrackConstraintSetMap::set(MediaConstraintType constraintType, Optional<IntConstraint>&& constraint)
{
    switch (constraintType) {
    case MediaConstraintType::Width:
        m_width = WTFMove(constraint);
        break;
    case MediaConstraintType::Height:
        m_height = WTFMove(constraint);
        break;
    case MediaConstraintType::SampleRate:
        m_sampleRate = WTFMove(constraint);
        break;
    case MediaConstraintType::SampleSize:
        m_sampleSize = WTFMove(constraint);
        break;

    case MediaConstraintType::AspectRatio:
    case MediaConstraintType::FrameRate:
    case MediaConstraintType::Volume:
    case MediaConstraintType::EchoCancellation:
    case MediaConstraintType::FacingMode:
    case MediaConstraintType::DeviceId:
    case MediaConstraintType::GroupId:
    case MediaConstraintType::Unknown:
        ASSERT_NOT_REACHED();
        break;
    }
}

void MediaTrackConstraintSetMap::set(MediaConstraintType constraintType, Optional<DoubleConstraint>&& constraint)
{
    switch (constraintType) {
    case MediaConstraintType::AspectRatio:
        m_aspectRatio = WTFMove(constraint);
        break;
    case MediaConstraintType::FrameRate:
        m_frameRate = WTFMove(constraint);
        break;
    case MediaConstraintType::Volume:
        m_volume = WTFMove(constraint);
        break;

    case MediaConstraintType::Width:
    case MediaConstraintType::Height:
    case MediaConstraintType::SampleRate:
    case MediaConstraintType::SampleSize:
    case MediaConstraintType::EchoCancellation:
    case MediaConstraintType::FacingMode:
    case MediaConstraintType::DeviceId:
    case MediaConstraintType::GroupId:
    case MediaConstraintType::Unknown:
        ASSERT_NOT_REACHED();
        break;
    }
}

void MediaTrackConstraintSetMap::set(MediaConstraintType constraintType, Optional<BooleanConstraint>&& constraint)
{
    switch (constraintType) {
    case MediaConstraintType::EchoCancellation:
        m_echoCancellation = WTFMove(constraint);
        break;

    case MediaConstraintType::Width:
    case MediaConstraintType::Height:
    case MediaConstraintType::SampleRate:
    case MediaConstraintType::SampleSize:
    case MediaConstraintType::AspectRatio:
    case MediaConstraintType::FrameRate:
    case MediaConstraintType::Volume:
    case MediaConstraintType::FacingMode:
    case MediaConstraintType::DeviceId:
    case MediaConstraintType::GroupId:
    case MediaConstraintType::Unknown:
        ASSERT_NOT_REACHED();
        break;
    }
}

void MediaTrackConstraintSetMap::set(MediaConstraintType constraintType, Optional<StringConstraint>&& constraint)
{
    switch (constraintType) {
    case MediaConstraintType::FacingMode:
        m_facingMode = WTFMove(constraint);
        break;
    case MediaConstraintType::DeviceId:
        m_deviceId = WTFMove(constraint);
        break;
    case MediaConstraintType::GroupId:
        m_groupId = WTFMove(constraint);
        break;

    // Legacy Constraints - for compatibility
    case MediaConstraintType::MinAspectRatio:
        m_minAspectRatio = WTFMove(constraint);
        break;
    case MediaConstraintType::MaxAspectRatio:
        m_maxAspectRatio = WTFMove(constraint);
        break;
    case MediaConstraintType::MaxWidth:
        m_maxWidth = WTFMove(constraint);
        break;
    case MediaConstraintType::MinWidth:
        m_minWidth = WTFMove(constraint);
        break;
    case MediaConstraintType::MaxHeight:
        m_maxHeight = WTFMove(constraint);
        break;
    case MediaConstraintType::MinHeight:
        m_minHeight = WTFMove(constraint);
        break;
    case MediaConstraintType::MaxFrameRate:
        m_maxFrameRate = WTFMove(constraint);
        break;
    case MediaConstraintType::MinFrameRate:
        m_minFrameRate = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogEchoCancellation:
        m_googEchoCancellation = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogEchoCancellation2:
        m_googEchoCancellation2 = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogDAEchoCancellation:
        m_googDAEchoCancellation = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogAutoGainControl:
        m_googAutoGainControl = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogAutoGainControl2:
        m_googAutoGainControl2 = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogNoiseSuppression:
        m_googNoiseSuppression = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogNoiseSuppression2:
        m_googNoiseSuppression2 = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogHighpassFilter:
        m_googHighpassFilter = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogTypingNoiseDetection:
        m_googTypingNoiseDetection = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogAudioMirroring:
        m_googAudioMirroring = WTFMove(constraint);
        break;
    case MediaConstraintType::AudioDebugRecording:
        m_audioDebugRecording = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogNoiseReduction:
        m_googNoiseReduction = WTFMove(constraint);
        break;
    case MediaConstraintType::OfferToReceiveAudio:
        m_offerToReceiveAudio = WTFMove(constraint);
        break;
    case MediaConstraintType::OfferToReceiveVideo:
        m_offerToReceiveVideo = WTFMove(constraint);
        break;
    case MediaConstraintType::VoiceActivityDetection:
        m_voiceActivityDetection = WTFMove(constraint);
        break;
    case MediaConstraintType::IceRestart:
        m_iceRestart = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogUseRtpMUX:
        m_googUseRtpMUX = WTFMove(constraint);
        break;
    case MediaConstraintType::DtlsSrtpKeyAgreement:
        m_dtlsSrtpKeyAgreement = WTFMove(constraint);
        break;
    case MediaConstraintType::RtpDataChannels:
        m_rtpDataChannels = WTFMove(constraint);
        break;
    case MediaConstraintType::Preferh264:
        m_preferh264 = WTFMove(constraint);
        break;
    case MediaConstraintType::IgnoreInactiveInterfaces:
        m_ignoreInactiveInterfaces = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogDscp:
        m_googDscp = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogIPv6:
        m_googIPv6 = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogSuspendBelowMinBitrate:
        m_googSuspendBelowMinBitrate = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogNumUnsignalledRecvStreams:
        m_googNumUnsignalledRecvStreams = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogCombinedAudioVideoBwe:
        m_googCombinedAudioVideoBwe = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogScreencastMinBitrate:
        m_googScreencastMinBitrate = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogCpuOveruseDetection:
        m_googCpuOveruseDetection = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogCpuUnderuseThreshold:
        m_googCpuUnderuseThreshold = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogCpuOveruseThreshold:
        m_googCpuOveruseThreshold = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogCpuUnderuseEncodeRsdThreshold:
        m_googCpuUnderuseEncodeRsdThreshold = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogCpuOveruseEncodeRsdThreshold:
        m_googCpuOveruseEncodeRsdThreshold = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogCpuOveruseEncodeUsage:
        m_googCpuOveruseEncodeUsage = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogHighStartBitrate:
        m_googHighStartBitrate = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogHighBitrate:
        m_googHighBitrate = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogVeryHighBitrate:
        m_googVeryHighBitrate = WTFMove(constraint);
        break;
    case MediaConstraintType::GoogPayloadPadding:
        m_googPayloadPadding = WTFMove(constraint);
        break;

    case MediaConstraintType::Width:
    case MediaConstraintType::Height:
    case MediaConstraintType::SampleRate:
    case MediaConstraintType::SampleSize:
    case MediaConstraintType::AspectRatio:
    case MediaConstraintType::FrameRate:
    case MediaConstraintType::Volume:
    case MediaConstraintType::EchoCancellation:
    case MediaConstraintType::Unknown:
        ASSERT_NOT_REACHED();
        break;
    }
}

size_t MediaTrackConstraintSetMap::size() const
{
    size_t count = 0;
    forEach([&count] (const MediaConstraint&) mutable {
        ++count;
    });

    return count;
}

bool MediaTrackConstraintSetMap::isEmpty() const
{
    return !size();
}

}

#endif // ENABLE(MEDIA_STREAM)
