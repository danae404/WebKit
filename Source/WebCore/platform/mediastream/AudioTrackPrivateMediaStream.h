/*
 * Copyright (C) 2015-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(MEDIA_STREAM)

#include "AudioTrackPrivate.h"
#include "MediaStreamTrackPrivate.h"
#include <wtf/CheckedRef.h>
#include <wtf/TZoneMalloc.h>

namespace WebCore {

class AudioMediaStreamTrackRenderer;

class AudioTrackPrivateMediaStream final
    : public AudioTrackPrivate
    , public MediaStreamTrackPrivateObserver
    , private RealtimeMediaSource::AudioSampleObserver
    , public CanMakeCheckedPtr<AudioTrackPrivateMediaStream> {
    WTF_MAKE_TZONE_ALLOCATED(AudioTrackPrivateMediaStream);
    WTF_MAKE_NONCOPYABLE(AudioTrackPrivateMediaStream)
    WTF_OVERRIDE_DELETE_FOR_CHECKED_PTR(AudioTrackPrivateMediaStream);
public:
    static Ref<AudioTrackPrivateMediaStream> create(MediaStreamTrackPrivate& streamTrack)
    {
        return adoptRef(*new AudioTrackPrivateMediaStream(streamTrack));
    }
    ~AudioTrackPrivateMediaStream();

    void setTrackIndex(int index) { m_index = index; }
    void setAudioOutputDevice(const String&);

    MediaStreamTrackPrivate& streamTrack() { return m_streamTrack.get(); }
    Ref<MediaStreamTrackPrivate> protectedStreamTrack() { return m_streamTrack; }

    void clear();

    void play();
    void pause();
    bool isPlaying() const { return m_isPlaying; }
    bool shouldPlay() const { return m_shouldPlay; }

    void setVolume(float);
    float volume() const;

    void setMuted(bool);
    bool muted() const { return m_muted; }

#if !RELEASE_LOG_DISABLED
    ASCIILiteral logClassName() const final { return "AudioTrackPrivateMediaStream"_s; }
#endif

private:
    explicit AudioTrackPrivateMediaStream(MediaStreamTrackPrivate&);

    // CheckedPtr interface
    uint32_t checkedPtrCount() const final { return CanMakeCheckedPtr::checkedPtrCount(); }
    uint32_t checkedPtrCountWithoutThreadCheck() const final { return CanMakeCheckedPtr::checkedPtrCountWithoutThreadCheck(); }
    void incrementCheckedPtrCount() const final { CanMakeCheckedPtr::incrementCheckedPtrCount(); }
    void decrementCheckedPtrCount() const final { CanMakeCheckedPtr::decrementCheckedPtrCount(); }

    static RefPtr<AudioMediaStreamTrackRenderer> createRenderer(AudioTrackPrivateMediaStream&);

    // AudioTrackPrivate
    Kind kind() const final { return Kind::Main; }
    std::optional<AtomString> trackUID() const { return AtomString { m_streamTrack->id() }; }
    AtomString label() const final { return AtomString { m_streamTrack->label() }; }
    int trackIndex() const final { return m_index; }
    bool isBackedByMediaStreamTrack() const final { return true; }

    // MediaStreamTrackPrivateObserver
    void trackEnded(MediaStreamTrackPrivate&) final;
    void trackMutedChanged(MediaStreamTrackPrivate&)  final;
    void trackEnabledChanged(MediaStreamTrackPrivate&)  final;
    void trackSettingsChanged(MediaStreamTrackPrivate&) final { }

    // RealtimeMediaSource::AudioSampleObserver
    void audioSamplesAvailable(const MediaTime&, const PlatformAudioData&, const AudioStreamDescription&, size_t) final;

    void startRenderer();
    void stopRenderer();
    void updateRenderer();
    void createNewRenderer();

    // Main thread writable members
    bool m_isPlaying { false };
    bool m_shouldPlay { false };
    bool m_muted { false };
    bool m_isCleared { false };

    const Ref<MediaStreamTrackPrivate> m_streamTrack;
    const Ref<RealtimeMediaSource> m_audioSource;
    int m_index { 0 };

    // Audio thread members
    RefPtr<AudioMediaStreamTrackRenderer> m_renderer;
};

}

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::AudioTrackPrivateMediaStream)
    static bool isType(const WebCore::AudioTrackPrivate& track) { return track.isBackedByMediaStreamTrack(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // ENABLE(MEDIA_STREAM)
