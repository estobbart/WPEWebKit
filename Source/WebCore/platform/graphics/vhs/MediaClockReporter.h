#ifndef MediaClockReporter_h
#define MediaClockReporter_h

#if ENABLE(MEDIA_SOURCE) && USE(VHS)

#include "Timer.h"

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class MediaPlayer;

/**
 * Interface for the MediaClockReporter required to become active.
 */
class MediaClockDataSource {
public:
    virtual float lastPresentedTimestamp() = 0;
};

/**
 * MediaClockReporter is an object that calls MediaPlayer methods without
 * having direct access to the MediaPlayer.
 * It also helps by limiting the number of methods on MediaPlayerPrivate
 * that only act as a pass through to the MediaPlayer.
 */
class MediaClockReporter : public RefCounted<MediaClockReporter> {

public:
    static RefPtr<MediaClockReporter> create(MediaPlayer *mediaPlayer);
    virtual ~MediaClockReporter();

    // TODO: Need to be able to check the duration from the MediaSourceClient
    // check the position from the platform..
    void activateReporter(MediaClockDataSource *clockSource);
    void deactivateReporter();

    bool isActive() const { return !!m_clockSource; }

    // durationChange updates to the MediaPlayer, the caller of durationChange
    // should track the reported values to be sure
    void durationChanged() const;

    // TODO: MediaTime or float?
    float currentTime() const;

    // callback provided to the 250ms timer
    void reportTime();

    // getter/setter for if the player is processing a seek
    void setSeeking(bool seeking) { m_seeking = seeking; }
    bool seeking() const { return m_seeking; }

private:
    MediaClockReporter(MediaPlayer *);

    // TODO: RefPtr this?
    /**
     * MediaPlayer is the object that receives updates.
     */
    MediaPlayer *m_mediaPlayer;

    /**
     * MediaClockDataSource
     */
    MediaClockDataSource *m_clockSource;

    /**
     * Timer is used to monitor the current position of the content
     * and report those changes to the MediaPlayer.
     */
    Timer m_intervalTimer;

    /**
     * Tracks the time reported to make sure we don't over dispatch this event.
     */
    float m_lastReportedTime;

    /**
     * flag for if the player is processing a seek request
     */
    bool m_seeking;
};

}

#endif // ENABLE(MEDIA_SOURCE) && USE(VHS)

#endif
