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

#ifndef _PEERCONNECTIONBACKENDQT5WEBRTC_H_
#define _PEERCONNECTIONBACKENDQT5WEBRTC_H_

#include "PeerConnectionBackend.h"
#include "NotImplemented.h"

#include "Dictionary.h"
#include "RTCDataChannelHandler.h"
#include "RealtimeMediaSourceCenterQt5WebRTC.h"

#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class PeerConnectionBackendClient;
class RTCDataChannel;

class PeerConnectionBackendQt5WebRTC : public PeerConnectionBackend, public WRTCInt::RTCPeerConnectionClient {
public:
    //PeerConnectionBackendQt5WebRTC(PeerConnectionBackendClient*);
    PeerConnectionBackendQt5WebRTC(RTCPeerConnection& client);

    //virtual void createOffer(RTCOfferOptions&, PeerConnection::SessionDescriptionPromise&&) override;
    //virtual void createAnswer(RTCAnswerOptions&, PeerConnection::SessionDescriptionPromise&&) override;

    //virtual void setLocalDescription(RTCSessionDescription&, PeerConnection::VoidPromise&&) override;
    virtual RefPtr<RTCSessionDescription> localDescription() const override;
    virtual RefPtr<RTCSessionDescription> currentLocalDescription() const override;
    virtual RefPtr<RTCSessionDescription> pendingLocalDescription() const override;

    //virtual void setRemoteDescription(RTCSessionDescription&, PeerConnection::VoidPromise&&) override;
    virtual RefPtr<RTCSessionDescription> remoteDescription() const override;
    virtual RefPtr<RTCSessionDescription> currentRemoteDescription() const override;
    virtual RefPtr<RTCSessionDescription> pendingRemoteDescription() const override;

    virtual void setConfiguration(RTCConfiguration&, const MediaConstraints&) override;
    //virtual void addIceCandidate(RTCIceCandidate&, PeerConnection::VoidPromise&&) override;

    virtual std::unique_ptr<RTCDataChannelHandler> createDataChannelHandler(const String&, const RTCDataChannelInit&) override;

    virtual void getStats(MediaStreamTrack*, PeerConnection::StatsPromise&&) override;

    virtual Vector<RefPtr<MediaStream>> getRemoteStreams() const override;
    virtual Ref/*Ptr*/<RTCRtpReceiver> createReceiver(const String& transceiverMid, const String& trackKind, const String& trackId) override;
    virtual void replaceTrack(RTCRtpSender&, RefPtr<MediaStreamTrack>&&, PeerConnection::VoidPromise&&) override;

    //virtual void stop() override;

    virtual bool isNegotiationNeeded() const override;
    virtual void markAsNeedingNegotiation() override;
    virtual void clearNegotiationNeededState() override;

    virtual std::unique_ptr<RTCDataChannelHandler> createDataChannel(const String&, const Dictionary&) override;

    virtual void emulatePlatformEvent(const String& /*action*/) override {}

    // WRTCInt::RTCPeerConnectionClient
    virtual void requestSucceeded(int id, const WRTCInt::RTCSessionDescription& desc) override;
    virtual void requestSucceeded(int id, const std::vector<std::unique_ptr<WRTCInt::RTCStatsReport>>& reports) override;
    virtual void requestSucceeded(int id) override;
    virtual void requestFailed(int id, const std::string& error) override;
    virtual void negotiationNeeded() override;
    virtual void didAddRemoteStream(WRTCInt::RTCMediaStream *stream,
                                    const std::vector<std::string> &audioSources,
                                    const std::vector<std::string> &videoSources) override;
    virtual void didGenerateIceCandidate(const WRTCInt::RTCIceCandidate& candidate) override;

    virtual void didChangeSignalingState(WRTCInt::SignalingState state) override;
    virtual void didChangeIceGatheringState(WRTCInt::IceGatheringState state) override;
    virtual void didChangeIceConnectionState(WRTCInt::IceConnectionState state) override;
    virtual void didAddRemoteDataChannel(WRTCInt::RTCDataChannel* channel) override;

private:
    virtual void doCreateOffer(RTCOfferOptions&&) override;
    virtual void doCreateAnswer(RTCAnswerOptions&&) override;
    virtual void doSetLocalDescription(RTCSessionDescription&) override;
    virtual void doSetRemoteDescription(RTCSessionDescription&) override;
    virtual void doAddIceCandidate(RTCIceCandidate&) override;
    virtual void doStop() override;

    //PeerConnectionBackendClient* m_client;
    std::unique_ptr<WRTCInt::RTCPeerConnection> m_rtcConnection;

    bool m_isNegotiationNeeded { false };
    int m_sessionDescriptionRequestId { WRTCInt::InvalidRequestId };
    //Optional<PeerConnection::SessionDescriptionPromise> m_sessionDescriptionPromise;
    int m_voidRequestId { WRTCInt::InvalidRequestId };
    //Optional<PeerConnection::VoidPromise> m_voidPromise;
    HashMap<int, Optional<PeerConnection::StatsPromise>> m_statsPromises;
    //Vector<Ref<RTCDataChannel>> m_remoteDataChannels;
    Vector<RefPtr<MediaStream>> m_remoteStreams;
};

class RTCDataChannelHandlerQt5WebRTC
    : public RTCDataChannelHandler
    , public WRTCInt::RTCDataChannelClient
{
public:
    RTCDataChannelHandlerQt5WebRTC(WRTCInt::RTCDataChannel* dataChannel);

    // RTCDataChannelHandler
    void setClient(RTCDataChannelHandlerClient*) override;
    /*String label() override;
    bool ordered() override;
    unsigned short maxRetransmitTime() override;
    unsigned short maxRetransmits() override;
    String protocol() override;
    bool negotiated() override;
    unsigned short id() override;
    unsigned long bufferedAmount() override;*/
    bool sendStringData(const String&) override;
    bool sendRawData(const char*, size_t) override;
    void close() override;

    // WRTCInt::RTCDataChannelClient
    void didChangeReadyState(WRTCInt::DataChannelState state) override;
    void didReceiveStringData(const std::string& str) override;
    void didReceiveRawData(const char* data, size_t sz) override;

private:
    std::unique_ptr<WRTCInt::RTCDataChannel> m_rtcDataChannel;
    RTCDataChannelHandlerClient* m_client;
};

}
#endif  // _PEERCONNECTIONBACKENDQT5WEBRTC_H_
