#duration values & changes

Duration values are reported by the SourceBufferPrivateVHS when it returns
it's initializationSegment back to the SourceBufferPrivateClient (SourceBuffer).
The duration value is in the TKHD, MVHD, & MDHD boxes of the mp4.

The MediaSourcePrivateVHS then receives a durationChange call, and reports this
back through the MediaPlayerPrivateVHS who reports back to the MediaPlayer &
the HTMLMediaElement.

MediaSourcePrivateClient is the true source for the proper duration value.

Since two initialization segments are reported, two calls are made to the MediaSourcePrivateVHS
MediaPlayerPrivateVHS keeps a duration value only to filter out reporting
multiple durationChange events from reaching MediaPlayer & HTMLMediaElement.

#current time values, timeupdate events
TODO:

#HTMLMediaElement.readyState

readyState from HAVE_NOTHING -> HAVE_METADATA can be set by a SourceBuffer,
but since it's only actually set on one of the private buffers, the
SourceBufferPrivateVHS handles this instead and reports it back through the DataProxy
to the MediaSourcePrivate.

The remaining state transitions HAVE_CURRENT_DATA, HAVE_FUTURE_DATA, & HAVE_ENOUGH_DATA
occur in the MediaSource `monitorSurceBuffers` method, which happens after any
buffer appends a segment. The MediaSource sets the state on it's MediaSourcePrivate
pointer, so it's still the responsibility of the MediaSourcePrivateVHS to signal
these changes back to the MediaPlayer via a `readyStateChanged` call.

#HTMLMediaElement.networkState
TODO:

#WebKit Notes

if you use new/delete make sure to include config.h at the top of the file

#Helpful docs

https://webkit.org

https://webkit.org/blog/25/webkit-coding-style-guidelines/
https://webkit.org/code-style-guidelines/

https://webkit.org/blog/5381/refptr-basics/

RefPtr to Ref conversion style:

`*ref` not taking ownership

`copyRef()` taking ownership


# VHS Not Yet Supported:

- EME / ClearKey
- Playback rate > 1 || rate < 0
- Seeking outside of the buffered range
- Single source buffer configuration (multiple tracks single SourceBuffer)
- PTS rollover
