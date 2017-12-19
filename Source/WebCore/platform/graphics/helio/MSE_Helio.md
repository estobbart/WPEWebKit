#duration values & changes

Duration values are reported by the SourceBufferPrivateHelio when it returns
it's initializationSegment back to the SourceBufferPrivateClient (SourceBuffer).
The duration value is in the TKHD, MVHD, & MDHD boxes of the mp4.

The MediaSourcePrivateHelio then receives a durationChange call, and reports this
back through the MediaPlayerPrivateHelio who reports back to the MediaPlayer &
the HTMLMediaElement.

MediaSourcePrivateClient is the true source for the proper duration value.

Since two initialization segments are reported, two calls are made to the MediaSourcePrivateHelio
MediaPlayerPrivateHelio keeps a duration value only to filter out reporting
multiple durationChange events from reaching MediaPlayer & HTMLMediaElement.
