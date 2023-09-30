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

template<typename TraceEvent>
class EventQueue;

template<typename TraceEvent>
void flushEvents(const Utils::span<TraceEvent> events,
                 std::thread::id threadId,
                 EventQueue<TraceEvent> &eventQueue);
extern template void flushEvents(const Utils::span<StringViewTraceEvent> events,
                                 std::thread::id threadId,
                                 EventQueue<StringViewTraceEvent> &eventQueue);
extern template void flushEvents(const Utils::span<StringTraceEvent> events,
                                 std::thread::id threadId,
                                 EventQueue<StringTraceEvent> &eventQueue);

template<typename TraceEvent>
void flushInThread(EventQueue<TraceEvent> &eventQueue);
extern template void flushInThread(EventQueue<StringViewTraceEvent> &eventQueue);
extern template void flushInThread(EventQueue<StringTraceEvent> &eventQueue);

void openFile(class TraceFile &file);
void finalizeFile(class TraceFile &file);

class TraceFile
{
public:
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

template<typename TraceEvent>
class EventQueue
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

    TraceFile *file = nullptr;
    TraceEventsSpan eventsOne;
    TraceEventsSpan eventsTwo;
    TraceEventsSpan currentEvents;
    std::size_t eventsIndex = 0;
    IsEnabled isEnabled = IsEnabled::No;
};

template<typename TraceEvent, std::size_t eventCount>
class EventQueueData
{
    using TraceEvents = std::array<TraceEvent, eventCount>;

public:
    EventQueueData(TraceFile &file)
        : file{file}
    {}

    TraceFile &file;
    TraceEvents eventsOne;
    TraceEvents eventsTwo;
};

template<typename TraceEvent, std::size_t eventCount>
struct EventQueueDataPointer
{
    operator EventQueue<TraceEvent>() const
    {
        if constexpr (isTracerActive()) {
            return {&data->file, data->eventsOne, data->eventsTwo, data->eventsOne, 0, IsEnabled::Yes};
        } else {
            return {};
        }
    }

    std::unique_ptr<EventQueueData<TraceEvent, eventCount>> data;
};

template<typename TraceEvent, std::size_t eventCount>
EventQueueDataPointer<TraceEvent, eventCount> makeEventQueueData(TraceFile &file)
{
    if constexpr (isTracerActive()) {
        return {std::make_unique<EventQueueData<TraceEvent, eventCount>>(file)};
    } else {
        return {};
    }
}

EventQueue<StringTraceEvent> &globalEventQueue();

template<typename TraceEvent>
TraceEvent &getTraceEvent(EventQueue<TraceEvent> &eventQueue)
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

template<typename TraceEvent>
class Category
{
public:
    using IsActive = std::true_type;
    TracerLiteral name;
    EventQueue<TraceEvent> &eventQueue;
};

using StringViewCategory = Category<StringViewTraceEvent>;
using StringCategory = Category<StringTraceEvent>;

class DisabledCategory
{};

template<typename TraceEvent>
Category(TracerLiteral name, EventQueue<TraceEvent> &eventQueue) -> Category<TraceEvent>;

template<typename Category>
class Tracer
{
public:
    constexpr Tracer(TracerLiteral, Category &, TracerLiteral) {}
    constexpr Tracer(TracerLiteral, Category &) {}

    ~Tracer() {}
};

template<typename String>
class BasicTracer
{};

template<>
class Tracer<StringViewCategory>
{
public:
    constexpr Tracer(TracerLiteral name, StringViewCategory &category, TracerLiteral arguments)
        : m_name{name}
        , m_arguments{arguments}
        , m_category{category}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue.isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    constexpr Tracer(TracerLiteral name, StringViewCategory &category)
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
    StringViewCategory &m_category;
};

template<>
class Tracer<StringCategory>
{
public:
    Tracer(std::string name, StringViewCategory &category, std::string arguments)
        : m_name{std::move(name)}
        , m_arguments{arguments}
        , m_category{category}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue.isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    Tracer(std::string name, StringViewCategory &category)
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
    StringViewCategory &m_category;
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
