// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nanotraceglobals.h"

#include <utils/span.h>

#include <array>
#include <chrono>
#include <fstream>
#include <future>
#include <mutex>
#include <string>
#include <string_view>

namespace NanotraceHR {
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = std::chrono::nanoseconds;

static_assert(Clock::is_steady, "clock should be steady");
static_assert(std::is_same_v<Clock::duration, std::chrono::nanoseconds>,
              "the steady clock should have nano second resolution");

constexpr bool isTracerActive()
{
#ifdef NANOTRACE_ENABLED
    return true;
#else
    return false;
#endif
}

template<std::size_t size>
std::string_view toStringView(Utils::span<const char, size> string)
{
    return {string.data(), string.size()};
}

template<typename String>
struct TraceEvent
{
    TraceEvent() = default;
    TraceEvent(const TraceEvent &) = delete;
    TraceEvent(TraceEvent &&) = delete;
    TraceEvent &operator=(const TraceEvent &) = delete;
    TraceEvent &operator=(TraceEvent &&) = delete;
    ~TraceEvent() = default;

    String name;
    String category;
    String arguments;
    TimePoint start;
    Duration duration;
};

using StringViewTraceEvent = TraceEvent<std::string_view>;
using StringTraceEvent = TraceEvent<std::string>;

enum class IsEnabled { No, Yes };

template<typename TraceEvent, typename Enabled>
class EventQueue;

template<typename TraceEvent>
using EnabledEventQueue = EventQueue<TraceEvent, std::true_type>;

template<typename TraceEvent>
void flushEvents(const Utils::span<TraceEvent> events,
                 std::thread::id threadId,
                 EnabledEventQueue<TraceEvent> &eventQueue);
extern template void flushEvents(const Utils::span<StringViewTraceEvent> events,
                                 std::thread::id threadId,
                                 EnabledEventQueue<StringViewTraceEvent> &eventQueue);
extern template void flushEvents(const Utils::span<StringTraceEvent> events,
                                 std::thread::id threadId,
                                 EnabledEventQueue<StringTraceEvent> &eventQueue);

template<typename TraceEvent>
void flushInThread(EnabledEventQueue<TraceEvent> &eventQueue);
extern template void flushInThread(EnabledEventQueue<StringViewTraceEvent> &eventQueue);
extern template void flushInThread(EnabledEventQueue<StringTraceEvent> &eventQueue);

template<bool enable>
class TraceFile;

using EnabledTraceFile = TraceFile<true>;

void openFile(EnabledTraceFile &file);
void finalizeFile(EnabledTraceFile &file);

template<bool enable>
class TraceFile
{
public:
    using IsActive = std::false_type;

    TraceFile(std::string_view) {}
};

template<>
class TraceFile<true>
{
public:
    using IsActive = std::true_type;

    TraceFile([[maybe_unused]] std::string_view filePath)
        : filePath{filePath}
    {
        openFile(*this);
    }

    TraceFile(const TraceFile &) = delete;
    TraceFile(TraceFile &&) = delete;
    TraceFile &operator=(const TraceFile &) = delete;
    TraceFile &operator=(TraceFile &&) = delete;

    ~TraceFile() { finalizeFile(*this); }
    std::string filePath;
    std::mutex fileMutex;
    std::future<void> processing;
    std::ofstream out;
};

template<typename TraceEvent, typename Enabled>
class EventQueue
{};

template<typename TraceEvent>
class EventQueue<TraceEvent, std::true_type>
{
    using TraceEventsSpan = Utils::span<TraceEvent>;

public:
    EventQueue() = default;

    ~EventQueue()
    {
        if (isEnabled == IsEnabled::Yes)
            flushEvents(currentEvents, std::this_thread::get_id(), *this);
    }

    EventQueue(const EventQueue &) = delete;
    EventQueue(EventQueue &&) = delete;
    EventQueue &operator=(const EventQueue &) = delete;
    EventQueue &operator=(EventQueue &&) = delete;

    EnabledTraceFile *file = nullptr;
    TraceEventsSpan eventsOne;
    TraceEventsSpan eventsTwo;
    TraceEventsSpan currentEvents;
    std::size_t eventsIndex = 0;
    IsEnabled isEnabled = IsEnabled::No;
};

template<typename TraceEvent, std::size_t eventCount, typename Enabled>
class EventQueueData
{
    using TraceEvents = std::array<TraceEvent, eventCount>;

public:
    using IsActive = Enabled;

    EventQueueData(TraceFile<false> &) {}
};

template<typename TraceEvent, std::size_t eventCount>
class EventQueueData<TraceEvent, eventCount, std::true_type>
{
    using TraceEvents = std::array<TraceEvent, eventCount>;

public:
    using IsActive = std::true_type;

    EventQueueData(EnabledTraceFile &file)
        : file{file}
    {}

    EnabledTraceFile &file;
    TraceEvents eventsOne;
    TraceEvents eventsTwo;
};

template<typename TraceEvent, std::size_t eventCount, typename Enabled>
struct EventQueueDataPointer
{
    EventQueue<TraceEvent, std::false_type> createEventQueue() const { return {}; }
};

template<typename TraceEvent, std::size_t eventCount>
struct EventQueueDataPointer<TraceEvent, eventCount, std::true_type>
{
    EnabledEventQueue<TraceEvent> createEventQueue() const
    {
        if constexpr (isTracerActive()) {
            return {&data->file, data->eventsOne, data->eventsTwo, data->eventsOne, 0, IsEnabled::Yes};
        } else {
            return {};
        }
    }

    std::unique_ptr<EventQueueData<TraceEvent, eventCount, std::true_type>> data;
};

template<typename TraceEvent, std::size_t eventCount, typename TraceFile>
EventQueueDataPointer<TraceEvent, eventCount, typename TraceFile::IsActive> makeEventQueueData(
    TraceFile &file)
{
    if constexpr (isTracerActive() && std::is_same_v<typename TraceFile::IsActive, std::true_type>) {
        return {std::make_unique<EventQueueData<TraceEvent, eventCount, typename TraceFile::IsActive>>(
            file)};
    } else {
        return {};
    }
}

EnabledEventQueue<StringTraceEvent> &globalEventQueue();

template<typename TraceEvent>
TraceEvent &getTraceEvent(EnabledEventQueue<TraceEvent> &eventQueue)
{
    if (eventQueue.eventsIndex == eventQueue.currentEvents.size())
        flushInThread(eventQueue);

    return eventQueue.currentEvents[eventQueue.eventsIndex++];
}

namespace Literals {
struct TracerLiteral
{
    friend constexpr TracerLiteral operator""_t(const char *text, size_t size);

    constexpr operator std::string_view() const { return text; }

private:
    constexpr TracerLiteral(std::string_view text)
        : text{text}
    {}

    std::string_view text;
};

constexpr TracerLiteral operator""_t(const char *text, size_t size)
{
    return {std::string_view{text, size}};
}
} // namespace Literals

using namespace Literals;

template<typename TraceEvent, bool enabled>
class Category
{
public:
    using IsActive = std::false_type;

    Category(TracerLiteral, EventQueue<TraceEvent, std::true_type> &) {}

    Category(TracerLiteral, EventQueue<TraceEvent, std::false_type> &) {}
};

template<typename TraceEvent>
class Category<TraceEvent, true>
{
public:
    using IsActive = std::true_type;
    TracerLiteral name;
    EnabledEventQueue<TraceEvent> &eventQueue;
};

template<bool enabled>
using StringViewCategory = Category<StringViewTraceEvent, enabled>;
template<bool enabled>
using StringCategory = Category<StringTraceEvent, enabled>;

class DisabledCategory
{
    using IsActive = std::false_type;
};

template<typename Category>
class Tracer
{
public:
    constexpr Tracer(TracerLiteral, Category &, TracerLiteral) {}

    constexpr Tracer(TracerLiteral, Category &) {}

    ~Tracer() {}
};

template<>
class Tracer<StringViewCategory<true>>
{
public:
    constexpr Tracer(TracerLiteral name, StringViewCategory<true> &category, TracerLiteral arguments)
        : m_name{name}
        , m_arguments{arguments}
        , m_category{category}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue.isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    constexpr Tracer(TracerLiteral name, StringViewCategory<true> &category)
        : Tracer{name, category, "{}"_t}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue.isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    ~Tracer()
    {
        if constexpr (isTracerActive()) {
            if (m_category.eventQueue.isEnabled == IsEnabled::Yes) {
                auto duration = Clock::now() - m_start;
                auto &traceEvent = getTraceEvent(m_category.eventQueue);
                traceEvent.name = m_name;
                traceEvent.category = m_category.name;
                traceEvent.arguments = m_arguments;
                traceEvent.start = m_start;
                traceEvent.duration = duration;
            }
        }
    }

private:
    TimePoint m_start;
    std::string_view m_name;
    std::string_view m_arguments;
    StringViewCategory<true> &m_category;
};

template<>
class Tracer<StringCategory<true>>
{
public:
    Tracer(std::string name, StringViewCategory<true> &category, std::string arguments)
        : m_name{std::move(name)}
        , m_arguments{arguments}
        , m_category{category}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue.isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    Tracer(std::string name, StringViewCategory<true> &category)
        : Tracer{std::move(name), category, "{}"}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue.isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    ~Tracer()
    {
        if constexpr (isTracerActive()) {
            if (m_category.eventQueue.isEnabled == IsEnabled::Yes) {
                auto duration = Clock::now() - m_start;
                auto &traceEvent = getTraceEvent(m_category.eventQueue);
                traceEvent.name = std::move(m_name);
                traceEvent.category = m_category.name;
                traceEvent.arguments = std::move(m_arguments);
                traceEvent.start = m_start;
                traceEvent.duration = duration;
            }
        }
    }

private:
    TimePoint m_start;
    std::string m_name;
    std::string m_arguments;
    StringViewCategory<true> &m_category;
};

template<typename Category>
Tracer(TracerLiteral name, Category &category) -> Tracer<Category>;

class GlobalTracer
{
public:
    GlobalTracer(std::string name, std::string category, std::string arguments)
        : m_name{std::move(name)}
        , m_category{std::move(category)}
        , m_arguments{std::move(arguments)}
    {
        if constexpr (isTracerActive()) {
            if (globalEventQueue().isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    GlobalTracer(std::string name, std::string category)
        : GlobalTracer{std::move(name), std::move(category), "{}"}
    {}

    ~GlobalTracer()
    {
        if constexpr (isTracerActive()) {
            if (globalEventQueue().isEnabled == IsEnabled::Yes) {
                auto duration = Clock::now() - m_start;
                auto &traceEvent = getTraceEvent(globalEventQueue());
                traceEvent.name = std::move(m_name);
                traceEvent.category = std::move(m_category);
                traceEvent.arguments = std::move(m_arguments);
                traceEvent.start = std::move(m_start);
                traceEvent.duration = std::move(duration);
            }
        }
    }

private:
    TimePoint m_start;
    std::string m_name;
    std::string m_category;
    std::string m_arguments;
};

} // namespace NanotraceHR
