// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nanotraceglobals.h"

#include <utils/span.h>

#include <QMetaObject>

#include <array>
#include <atomic>
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

template<typename String>
struct TraceEvent
{
    using StringType = String;
    using ArgumentType = std::conditional_t<std::is_same_v<String, std::string_view>, TracerLiteral, String>;

    TraceEvent() = default;
    TraceEvent(const TraceEvent &) = delete;
    TraceEvent(TraceEvent &&) = delete;
    TraceEvent &operator=(const TraceEvent &) = delete;
    TraceEvent &operator=(TraceEvent &&) = delete;
    ~TraceEvent() = default;

    String name;
    String category;
    String arguments;
    TimePoint time;
    Duration duration;
    std::size_t id = 0;
    char type = ' ';
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
extern template NANOTRACE_EXPORT void flushEvents(const Utils::span<StringViewTraceEvent> events,
                                                  std::thread::id threadId,
                                                  EnabledEventQueue<StringViewTraceEvent> &eventQueue);
extern template NANOTRACE_EXPORT void flushEvents(const Utils::span<StringTraceEvent> events,
                                                  std::thread::id threadId,
                                                  EnabledEventQueue<StringTraceEvent> &eventQueue);

template<typename TraceEvent>
void flushInThread(EnabledEventQueue<TraceEvent> &eventQueue);
extern template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringViewTraceEvent> &eventQueue);
extern template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringTraceEvent> &eventQueue);

template<bool enable>
class TraceFile;

using EnabledTraceFile = TraceFile<true>;

NANOTRACE_EXPORT void openFile(EnabledTraceFile &file);
NANOTRACE_EXPORT void finalizeFile(EnabledTraceFile &file);

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
{
public:
    using IsActive = std::false_type;
};

template<typename TraceEvent>
class EventQueue<TraceEvent, std::true_type>
{
    using TraceEventsSpan = Utils::span<TraceEvent>;

public:
    using IsActive = std::true_type;

    EventQueue(EnabledTraceFile *file, TraceEventsSpan eventsOne, TraceEventsSpan eventsTwo);

    ~EventQueue();

    void flush();

    EventQueue(const EventQueue &) = delete;
    EventQueue(EventQueue &&) = delete;
    EventQueue &operator=(const EventQueue &) = delete;
    EventQueue &operator=(EventQueue &&) = delete;

    EnabledTraceFile *file = nullptr;
    TraceEventsSpan eventsOne;
    TraceEventsSpan eventsTwo;
    TraceEventsSpan currentEvents;
    std::size_t eventsIndex = 0;
    IsEnabled isEnabled = IsEnabled::Yes;
    QMetaObject::Connection connection;
    std::mutex mutex;
};

extern template class NANOTRACE_EXPORT EventQueue<StringViewTraceEvent, std::true_type>;
extern template class NANOTRACE_EXPORT EventQueue<StringTraceEvent, std::true_type>;

template<typename TraceEvent, std::size_t eventCount, typename Enabled>
class EventQueueData
{
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
            return {&data->file, data->eventsOne, data->eventsTwo};
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

NANOTRACE_EXPORT EnabledEventQueue<StringTraceEvent> &globalEventQueue();

template<typename TraceEvent>
TraceEvent &getTraceEvent(EnabledEventQueue<TraceEvent> &eventQueue)
{
    if (eventQueue.eventsIndex == eventQueue.currentEvents.size())
        flushInThread(eventQueue);

    return eventQueue.currentEvents[eventQueue.eventsIndex++];
}

template<typename Category, bool enabled>
class Token
{
public:
    using IsActive = std::false_type;
    using ArgumentType = typename Category::ArgumentType;

    Token() {}

    ~Token() {}

    constexpr Token(const Token &) = delete;
    constexpr Token &operator=(const Token &) = delete;

    constexpr Token(Token &&other) noexcept = default;

    constexpr Token &operator=(Token &&other) noexcept = default;

    constexpr explicit operator bool() const { return false; }

    static constexpr bool isActive() { return false; }

    Token begin(ArgumentType) { return Token{}; }

    void tick(ArgumentType) {}

    void end() {}
};

template<typename TraceEvent, bool enabled>
class Category;

template<typename Category>
class Token<Category, true>
{
    Token(std::string_view name, std::size_t id, Category &category)
        : m_name{name}
        , m_id{id}
        , m_category{&category}
    {}

public:
    using IsActive = std::true_type;
    using StringType = typename Category::StringType;
    using ArgumentType = typename Category::ArgumentType;

    friend Category;

    Token() = default;

    Token(const Token &) = delete;
    Token &operator=(const Token &) = delete;

    Token(Token &&other) noexcept
        : m_name{other.m_name}
        , m_id{std::exchange(other.m_id, 0)}
        , m_category{std::exchange(other.m_category, nullptr)}
    {}

    Token &operator=(Token &&other) noexcept
    {
        if (&other != this) {
            m_name = other.m_name;
            m_id = std::exchange(other.m_id, 0);
            m_category = std::exchange(other.m_category, nullptr);
        }

        return *this;
    }

    ~Token() { end(); }

    constexpr explicit operator bool() const { return m_id; }

    static constexpr bool isActive() { return true; }

    Token begin(ArgumentType name)
    {
        if (m_id)
            m_category->beginAsynchronous(m_id, name);

        return Token{m_name, m_id, *m_category};
    }

    void tick(ArgumentType name)
    {
        if (m_id)
            m_category->tickAsynchronous(m_id, name);
    }

    void end()
    {
        if (m_id)
            m_category->endAsynchronous(m_id, m_name);

        m_id = 0;
    }

private:
    StringType m_name;
    std::size_t m_id = 0;
    Category *m_category = nullptr;
};

template<typename TraceEvent, bool enabled>
class Category
{
public:
    using IsActive = std::false_type;
    using ArgumentType = typename TraceEvent::ArgumentType;
    using TokenType = Token<Category, false>;

    Category(ArgumentType, EventQueue<TraceEvent, std::true_type> &) {}

    Category(ArgumentType, EventQueue<TraceEvent, std::false_type> &) {}

    TokenType beginAsynchronous(ArgumentType) { return {}; }

    void tickAsynchronous(ArgumentType) {}

    static constexpr bool isActive() { return false; }
};

template<typename TraceEvent>
class Category<TraceEvent, true>
{
public:
    using IsActive = std::true_type;
    using ArgumentType = typename TraceEvent::ArgumentType;
    using StringType = typename TraceEvent::StringType;
    using TokenType = Token<Category, true>;

    friend TokenType;

    template<typename EventQueue>
    Category(ArgumentType name, EventQueue &queue)
        : m_name{std::move(name)}
        , m_eventQueue{queue}
    {
        static_assert(std::is_same_v<typename EventQueue::IsActive, std::true_type>,
                      "A active category is not possible with an inactive event queue!");
        idCounter = globalIdCounter += 1ULL << 32;
    }

    TokenType beginAsynchronous(ArgumentType traceName)
    {
        std::size_t id = ++idCounter;

        beginAsynchronous(id, traceName);

        return {traceName, id, *this};
    }

    EnabledEventQueue<TraceEvent> &eventQueue() const { return m_eventQueue; }

    std::string_view name() const { return m_name; }

    static constexpr bool isActive() { return true; }

private:
    void beginAsynchronous(std::size_t id, StringType traceName)
    {
        auto &traceEvent = getTraceEvent(m_eventQueue);
        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.type = 'b';
        traceEvent.id = id;
        traceEvent.time = Clock::now();
    }

    void tickAsynchronous(std::size_t id, StringType traceName)
    {
        auto time = Clock::now();
        auto &traceEvent = getTraceEvent(m_eventQueue);
        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.time = time;
        traceEvent.type = 'n';
        traceEvent.id = id;
    }

    void endAsynchronous(std::size_t id, StringType traceName)
    {
        auto time = Clock::now();
        auto &traceEvent = getTraceEvent(m_eventQueue);
        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.time = time;
        traceEvent.type = 'e';
        traceEvent.id = id;
    }

private:
    StringType m_name;
    EnabledEventQueue<TraceEvent> &m_eventQueue;
    inline static std::atomic<std::size_t> globalIdCounter;
    std::size_t idCounter;
};

template<bool enabled>
using StringViewCategory = Category<StringViewTraceEvent, enabled>;
template<bool enabled>
using StringCategory = Category<StringTraceEvent, enabled>;

template<typename Category>
class Tracer
{
public:
    using ArgumentType = typename Category::ArgumentType;

    constexpr Tracer(ArgumentType, Category &, ArgumentType) {}

    constexpr Tracer(ArgumentType, Category &) {}

    ~Tracer() {}
};

template<>
class Tracer<StringViewCategory<true>>
{
public:
    Tracer(TracerLiteral name, StringViewCategory<true> &category, TracerLiteral arguments)
        : m_name{name}
        , m_arguments{arguments}
        , m_category{category}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue().isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    Tracer(TracerLiteral name, StringViewCategory<true> &category)
        : Tracer{name, category, ""_t}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue().isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    ~Tracer()
    {
        if constexpr (isTracerActive()) {
            if (m_category.eventQueue().isEnabled == IsEnabled::Yes) {
                auto duration = Clock::now() - m_start;
                auto &traceEvent = getTraceEvent(m_category.eventQueue());
                traceEvent.name = m_name;
                traceEvent.category = m_category.name();
                traceEvent.arguments = m_arguments;
                traceEvent.time = m_start;
                traceEvent.duration = duration;
                traceEvent.type = 'X';
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
            if (category.eventQueue().isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    Tracer(std::string name, StringViewCategory<true> &category)
        : Tracer{std::move(name), category, ""}
    {
        if constexpr (isTracerActive()) {
            if (category.eventQueue().isEnabled == IsEnabled::Yes)
                m_start = Clock::now();
        }
    }

    ~Tracer()
    {
        if constexpr (isTracerActive()) {
            if (m_category.eventQueue().isEnabled == IsEnabled::Yes) {
                auto duration = Clock::now() - m_start;
                auto &traceEvent = getTraceEvent(m_category.eventQueue());
                traceEvent.name = std::move(m_name);
                traceEvent.category = m_category.name();
                traceEvent.arguments = std::move(m_arguments);
                traceEvent.time = m_start;
                traceEvent.duration = duration;
                traceEvent.type = 'X';
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
        : GlobalTracer{std::move(name), std::move(category), ""}
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
                traceEvent.time = std::move(m_start);
                traceEvent.duration = std::move(duration);
                traceEvent.type = 'X';
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
