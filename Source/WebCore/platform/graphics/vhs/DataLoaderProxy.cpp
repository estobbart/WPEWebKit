#include "config.h"
#include "DataLoaderProxy.h"

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include "SourceBufferPrivateVHS.h"
#include "ReadyStateResponder.h"

#include <stdio.h>

namespace WebCore {

RefPtr<DataLoaderProxy> DataLoaderProxy::create(ReadyStateResponder *readyStateResponder) {
   return adoptRef(new DataLoaderProxy(readyStateResponder));
}

DataLoaderProxy::DataLoaderProxy(ReadyStateResponder *readyStateResponder)
  : m_readyStateResponder(readyStateResponder)
  , m_loadedDataToggle(false) {
      printf("%s\n", __PRETTY_FUNCTION__);
}

DataLoaderProxy::~DataLoaderProxy() {
    printf("%s\n", __PRETTY_FUNCTION__);
}

bool DataLoaderProxy::didLoadingProgress() {
    bool tmpDidProgress = m_loadedDataToggle;
    m_loadedDataToggle = false;
    return tmpDidProgress;
}

void DataLoaderProxy::addSourceBuffer(RefPtr<SourceBufferPrivateVHS>&& sourceBuffer) {
    m_sourceBuffers.append(WTFMove(sourceBuffer));
}

void DataLoaderProxy::removeSourceBuffer(SourceBufferPrivateVHS *sourceBuffer) {
    size_t pos = m_sourceBuffers.find(sourceBuffer);
    m_sourceBuffers.remove(pos);
}

bool DataLoaderProxy::dataSourceHasAudio() const {
    return std::any_of(m_sourceBuffers.begin(), m_sourceBuffers.end(), [](auto& sourceBuffer) {
        return sourceBuffer->clientHasAudio();
    });
}

bool DataLoaderProxy::dataSourceHasVideo() const {
    return std::any_of(m_sourceBuffers.begin(), m_sourceBuffers.end(), [](auto& sourceBuffer) {
        return sourceBuffer->clientHasVideo();
    });
}

void DataLoaderProxy::sourceBufferDidTransition() const {
    if (std::all_of(m_sourceBuffers.begin(), m_sourceBuffers.end(), [](auto& sourceBuffer) {
        return sourceBuffer->readyState() == MediaPlayer::HaveMetadata;
    })) {
        m_readyStateResponder->setReadyState(MediaPlayer::HaveMetadata);
    }
}


} // namespace webcore

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)
