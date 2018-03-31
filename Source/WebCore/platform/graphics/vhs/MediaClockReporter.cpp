#include "config.h"
#include "MediaClockReporter.h"

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include "MediaPlayer.h"
#include <wtf/Seconds.h>

namespace WebCore {

RefPtr<MediaClockReporter> MediaClockReporter::create(MediaPlayer *mediaPlayer) {
   return adoptRef(new MediaClockReporter(mediaPlayer));
}

MediaClockReporter::MediaClockReporter(MediaPlayer *mediaPlayer)
  : m_mediaPlayer(mediaPlayer)
  , m_intervalTimer(*this, &MediaClockReporter::reportTime)
  , m_lastReportedTime(0.0)
  , m_seeking(false) {
}

MediaClockReporter::~MediaClockReporter() {
    deactivateReporter();
}

void MediaClockReporter::activateReporter(MediaClockDataSource *clockSource) {
    if (!clockSource) {
        return;
    }
    m_clockSource = clockSource;
    if (!m_intervalTimer.isActive()) {
        // 4.8.10.8 Playing the media resource
        /**
         * If the time was reached through the usual monotonic increase of the
         * current playback position during normal playback, and if the user
         * agent has not fired a timeupdate event at the element in the past 15
         * to 250ms and is not still running event handlers for such an event,
         * then the user agent must queue a task to fire a simple event named
         * timeupdate at the element. (In the other cases, such as explicit
         * seeks, relevant events get fired as part of the overall process of
         * changing the current playback position.)
         */
        m_intervalTimer.startRepeating(Seconds::fromMilliseconds(250));
    }
}

void MediaClockReporter::deactivateReporter() {
    m_intervalTimer.stop();
    m_clockSource = nullptr;
}

void MediaClockReporter::durationChanged() const {
    m_mediaPlayer->durationChanged();
}

void MediaClockReporter::reportTime() {
    if (m_seeking) {
        return;
    }
    float now = currentTime();
    if (m_lastReportedTime == now) {
        return;
    }
    m_lastReportedTime = now;
    m_mediaPlayer->timeChanged();
}

float MediaClockReporter::currentTime() const {
    if (m_clockSource) {
        return m_clockSource->lastPresentedTimestamp();
    }
    return 0.0;
}

} // namespace webcore

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)
