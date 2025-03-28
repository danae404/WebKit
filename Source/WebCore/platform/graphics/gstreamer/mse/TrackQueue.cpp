/*
 * Copyright (C) 2020, 2021 Metrological Group B.V.
 * Copyright (C) 2020, 2021 Igalia, S.L
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TrackQueue.h"

#if ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

GST_DEBUG_CATEGORY_STATIC(webkit_mse_track_queue_debug);
#define GST_CAT_DEFAULT webkit_mse_track_queue_debug

namespace WebCore {

TrackQueue::TrackQueue(TrackID trackId)
    : m_trackId(trackId)
{
    static std::once_flag debugRegisteredFlag;
    std::call_once(debugRegisteredFlag, [] {
        GST_DEBUG_CATEGORY_INIT(webkit_mse_track_queue_debug, "webkitmsetrackqueue", 0, "WebKit MSE TrackQueue");
    });
}

void TrackQueue::enqueueObject(GRefPtr<GstMiniObject>&& object)
{
    ASSERT(isMainThread());
    ASSERT(GST_IS_SAMPLE(object.get()) || GST_IS_EVENT(object.get()));

    if (GST_IS_SAMPLE(object.get())) {
        GST_TRACE("TrackQueue for '%" PRIu64 "': Putting object sample in the queue: %" GST_PTR_FORMAT " Buffer: %" GST_PTR_FORMAT ". notEmptyCallback currently %s.",
            m_trackId, object.get(), gst_sample_get_buffer(GST_SAMPLE(object.get())),
            m_notEmptyCallback ? "set, will be called" : "unset");
    } else {
        GST_DEBUG("TrackQueue for '%" PRIu64 "': Putting object event in the queue: %" GST_PTR_FORMAT ". notEmptyCallback currently %s.",
            m_trackId, object.get(),
            m_notEmptyCallback ? "set, will be called" : "unset");
    }
    if (!m_notEmptyCallback)
        m_queue.append(WTFMove(object));
    else {
        // If a low level callback was ever set, it had to be dispatched when the queue was empty at latest.
        ASSERT(!m_lowLevelCallback);

        NotEmptyHandler notEmptyCallback;
        std::swap(notEmptyCallback, m_notEmptyCallback);
        notEmptyCallback(WTFMove(object));
    }
}

void TrackQueue::clear()
{
    ASSERT(isMainThread());
    m_queue.clear();
    GST_DEBUG("TrackQueue for '%" PRIu64 "': Emptied.", m_trackId);
    // Notify main thread of low level reached if it proceeds.
    checkLowLevel();
}

void TrackQueue::flush()
{
    clear();
    // If there was a callback in the streaming thread waiting for a sample to be added, cancel it.
    if (m_notEmptyCallback) {
        m_notEmptyCallback = nullptr;
        GST_DEBUG("TrackQueue for '%" PRIu64 "': notEmptyCallback unset.", m_trackId);
    }
}

void TrackQueue::notifyWhenLowLevel(LowLevelHandler&& lowLevelCallback)
{
    ASSERT(isMainThread());
    GST_TRACE("TrackQueue for '%" PRIu64 "': Setting lowLevelCallback%s.", m_trackId,
        m_lowLevelCallback ? " (previous callback will be discarded)" : "");
    m_lowLevelCallback = WTFMove(lowLevelCallback);
    checkLowLevel();
}

GRefPtr<GstMiniObject> TrackQueue::pop()
{
    ASSERT(!isEmpty());
    GRefPtr<GstMiniObject> object = m_queue.takeFirst();
    if (GST_IS_SAMPLE(object.get())) {
        GST_TRACE("TrackQueue for '%" PRIu64 "': Popped object sample from the queue: %" GST_PTR_FORMAT " Buffer: %" GST_PTR_FORMAT,
            m_trackId, object.get(), gst_sample_get_buffer(GST_SAMPLE(object.get())));
    } else {
        GST_DEBUG("TrackQueue for '%" PRIu64 "': Popped object event from the queue: %" GST_PTR_FORMAT,
            m_trackId, object.get());
    }
    checkLowLevel();
    return object;
}

void TrackQueue::notifyWhenNotEmpty(NotEmptyHandler&& notEmptyCallback)
{
    ASSERT(!isMainThread());
    ASSERT(!m_notEmptyCallback);
    m_notEmptyCallback = WTFMove(notEmptyCallback);
    GST_TRACE("TrackQueue for '%" PRIu64 "': notEmptyCallback set.", m_trackId);
}

void TrackQueue::resetNotEmptyHandler()
{
    ASSERT(!isMainThread());
    if (!m_notEmptyCallback)
        return;
    m_notEmptyCallback = nullptr;
    GST_TRACE("TrackQueue for '%" PRIu64 "': notEmptyCallback reset.", m_trackId);
}

void TrackQueue::checkLowLevel()
{
    if (!m_lowLevelCallback || durationEnqueued() > s_durationEnqueuedLowWaterLevel)
        return;

    LowLevelHandler lowLevelCallback;
    std::swap(lowLevelCallback, m_lowLevelCallback);
    GST_TRACE("TrackQueue for '%" PRIu64 "': lowLevelCallback called.", m_trackId);
    lowLevelCallback();
}

GstClockTime TrackQueue::durationEnqueued() const
{
    // Find the first and last GstSample in the queue and subtract their DTS.

    auto frontIter = std::find_if(m_queue.begin(), m_queue.end(), [](const GRefPtr<GstMiniObject>& object) {
        return GST_IS_SAMPLE(object.get());
    });

    // If there are no samples in the queue, that makes total duration of enqueued frames of zero.
    if (frontIter == m_queue.end())
        return 0;

    auto backIter = std::find_if(m_queue.rbegin(), m_queue.rend(), [](const GRefPtr<GstMiniObject>& object) {
        return GST_IS_SAMPLE(object.get());
    });

    const GstBuffer* front = gst_sample_get_buffer(GST_SAMPLE(frontIter->get()));
    const GstBuffer* back = gst_sample_get_buffer(GST_SAMPLE(backIter->get()));
    return GST_BUFFER_DTS_OR_PTS(back) - GST_BUFFER_DTS_OR_PTS(front);
}

#undef GST_CAT_DEFAULT

} // namespace WebCore

#endif // ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)
