#ifndef DataLoaderProxy_h
#define DataLoaderProxy_h

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class SourceBufferPrivateVHS;
class ReadyStateResponder;

/**
 * TODO: This is a terrible name, change it.
 * DataLoaderProxy acts as an aggregator of the SourceBufferPrivateVHS objects.
 * It's owned by the MediaSourcePrivateVHS, read by the MediaPlayerPrivateVHS
 * and written to by SourceBufferPrivateVHS.
 *
 * TODO: Are two of these being created??!
 */
class DataLoaderProxy : public RefCounted<DataLoaderProxy> {

public:
    static RefPtr<DataLoaderProxy> create(ReadyStateResponder *readyStateResponder);
    virtual ~DataLoaderProxy();

    void addSourceBuffer(RefPtr<SourceBufferPrivateVHS>&& sourceBuffer);
    void removeSourceBuffer(SourceBufferPrivateVHS *sourceBuffer);

    bool didLoadingProgress();

    bool dataSourceHasAudio() const;
    bool dataSourceHasVideo() const;

private:
    DataLoaderProxy(ReadyStateResponder *readyStateResponder);

    void sourceBufferDidAppendData() { m_loadedDataToggle = true; }

    void sourceBufferDidTransition() const;

    friend class SourceBufferPrivateVHS;

    ReadyStateResponder *m_readyStateResponder;
    bool m_loadedDataToggle;
    Vector<RefPtr<SourceBufferPrivateVHS>> m_sourceBuffers;
};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)

#endif
