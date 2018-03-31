#ifndef ReadyStateResponder_h
#define ReadyStateResponder_h

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include "MediaPlayerEnums.h"

namespace WebCore {

class MediaSourcePrivateVHS;
class DataLoaderProxy;

class ReadyStateClient {
public:
    virtual void readyStateTransition() = 0;
};

/**
 * There are a lot of touch points for ReadyState in the MSE scenario.
 * ReadyStateResponder attempts to simplify this by having a single source
 * of truth for the state. ReadyState changes occur from the
 * MediaSourcePrivateVHS and are notified to the ReadyStateClient.
 */
class ReadyStateResponder : public RefCounted<ReadyStateResponder> {

public:
    static RefPtr<ReadyStateResponder> create(ReadyStateClient *);
    virtual ~ReadyStateResponder();

    MediaPlayerEnums::ReadyState readyState() const { return m_readyState; }

private:
    ReadyStateResponder(ReadyStateClient *);

    void setReadyState(MediaPlayerEnums::ReadyState);

    // updated state can come fom either of these
    friend class MediaSourcePrivateVHS;
    friend class DataLoaderProxy;

    MediaPlayerEnums::ReadyState m_readyState;

    ReadyStateClient *m_client;
};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)

#endif
