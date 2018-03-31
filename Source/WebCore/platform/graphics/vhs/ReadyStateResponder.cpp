#include "config.h"
#include "ReadyStateResponder.h"

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include <stdio.h>

namespace WebCore {

RefPtr<ReadyStateResponder> ReadyStateResponder::create(ReadyStateClient *client) {
   return adoptRef(new ReadyStateResponder(client));
}

ReadyStateResponder::ReadyStateResponder(ReadyStateClient *client)
  : m_readyState(MediaPlayerEnums::HaveNothing)
  , m_client(client) {
      printf("%s\n", __PRETTY_FUNCTION__);
}

ReadyStateResponder::~ReadyStateResponder() {
    printf("%s\n", __PRETTY_FUNCTION__);
}

void ReadyStateResponder::setReadyState(MediaPlayerEnums::ReadyState state) {
    if (m_readyState == state)
        return;

    printf("%s %i\n", __PRETTY_FUNCTION__, state);
    m_readyState = state;
    m_client->readyStateTransition();
}

} // namespace webcore

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)
