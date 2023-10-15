// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nanotraceglobals.h"

#include <utils/smallstring.h>
#include <utils/span.h>

#include <QByteArrayView>
#include <QMetaObject>
#include <QStringView>

#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <future>
#include <mutex>
#include <string>
#include <string_view>
#include <tuple>

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

#if __cplusplus >= 202002L && __has_cpp_attribute(msvc::no_unique_address)
#  define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif __cplusplus >= 202002L && __has_cpp_attribute(no_unique_address) >= 201803L
#  define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#  define NO_UNIQUE_ADDRESS
#endif

namespace Internal {
inline std::string_view covertToString(std::string_view text)
{
    return text;
}

template<std::size_t size>
inline std::string_view covertToString(const char (&text)[size])
{
    return text;
}

inline Utils::PathString covertToString(QStringView text)
{
    return text;
}

inline Utils::PathString covertToString(QByteArrayView text)
{
    return text;
}

inline Utils::SmallString covertToString(long long number)
{
    return Utils::SmallString::number(number);
}

inline Utils::SmallString covertToString(double number)
{
    return Utils::SmallString::number(number);
}

template<typename String, typename Argument>
void toArgument(String &text, Argument &&argument)
{
    const auto &[key, value] = argument;
    text.append(R"(")");
    text.append(covertToString(key));
    text.append(R"(":")");
    text.append(covertToString(value));
    text.append(R"(",)");
}

template<typename String, typename... Arguments>
String toArguments(Arguments &&...arguments)
{
    if constexpr (isTracerActive()) {
        String text;
        constexpr auto argumentCount = sizeof...(Arguments);
        text.reserve(sizeof...(Arguments) * 20);
        text.append("{");
        (toArgument(text, arguments), ...);
        if (argumentCount)
            text.pop_back();

        text.append("}");

        return text;
    } else {
        return {};
    }
}

inline std::string_view toArguments(std::string_view arguments)
{
    return arguments;
}

} // namespace Internal

namespace Literals {
struct TracerLiteral
{
    friend constexpr TracerLiteral operator""_t(const char *text, size_t size);

    constexpr operator std::string_view() const { return text; }

    operator std::string() const { return std::string{text}; }

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

template<typename String, typename ArgumentsString>
struct TraceEvent
{
    using StringType = String;
    using ArgumentType = std::conditional_t<std::is_same_v<String, std::string_view>, TracerLiteral, String>;
    using ArgumentsStringType = ArgumentsString;

    TraceEvent() = default;
    TraceEvent(const TraceEvent &) = delete;
    TraceEvent(TraceEvent &&) = delete;
    TraceEvent &operator=(const TraceEvent &) = delete;
    TraceEvent &operator=(TraceEvent &&) = delete;
    ~TraceEvent() = default;

    String name;
    String category;
    ArgumentsString arguments;
    TimePoint time;
    Duration duration;
    std::size_t id = 0;
    char type = ' ';
};

using StringViewTraceEvent = TraceEvent<std::string_view, std::string_view>;
using StringViewWithStringArgumentsTraceEvent = TraceEvent<std::string_view, std::string>;
using StringTraceEvent = TraceEvent<std::string, std::string>;

enum class IsEnabled { No, Yes };

template<typename TraceEvent, bool isEnabled>
class EventQueue;

template<typename TraceEvent>
using EnabledEventQueue = EventQueue<TraceEvent, true>;

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
extern template NANOTRACE_EXPORT void flushEvents(
    const Utils::span<StringViewWithStringArgumentsTraceEvent> events,
    std::thread::id threadId,
    EnabledEventQueue<StringViewWithStringArgumentsTraceEvent> &eventQueue);
template<typename TraceEvent>
void flushInThread(EnabledEventQueue<TraceEvent> &eventQueue);
extern template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringViewTraceEvent> &eventQueue);
extern template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringTraceEvent> &eventQueue);
extern template NANOTRACE_EXPORT void flushInThread(
    EnabledEventQueue<StringViewWithStringArgumentsTraceEvent> &eventQueue);

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

template<typename TraceEvent, bool isEnabled>
class EventQueue
{
public:
    using IsActive = std::false_type;
};

template<typename TraceEvent>
class EventQueue<TraceEvent, true>
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

extern template class NANOTRACE_EXPORT EventQueue<StringViewTraceEvent, true>;
extern template class NANOTRACE_EXPORT EventQueue<StringTraceEvent, true>;
extern template class NANOTRACE_EXPORT EventQueue<StringViewWithStringArgumentsTraceEvent, true>;

template<typename TraceEvent, std::size_t eventCount, bool isEnabled>
class EventQueueData
{
public:
    using IsActive = std::true_type;

    EventQueueData(TraceFile<false> &) {}

    EventQueue<TraceEvent, false> createEventQueue() { return {}; }
};

template<typename TraceEvent, std::size_t eventCount>
class EventQueueData<TraceEvent, eventCount, true>
{
    using TraceEvents = std::array<TraceEvent, eventCount>;

public:
    using IsActive = std::true_type;

    EventQueueData(EnabledTraceFile &file)
        : file{file}
    {}

    EventQueue<TraceEvent, true> createEventQueue() { return {&file, eventsOne, eventsTwo}; }

    EnabledTraceFile &file;
    TraceEvents eventsOne;
    TraceEvents eventsTwo;
};

NANOTRACE_EXPORT EventQueue<StringTraceEvent, isTracerActive()> &globalEventQueue();

template<typename TraceEvent>
TraceEvent &getTraceEvent(EnabledEventQueue<TraceEvent> &eventQueue)
{
    if (eventQueue.eventsIndex == eventQueue.currentEvents.size())
        flushInThread(eventQueue);

    return eventQueue.currentEvents[eventQueue.eventsIndex++];
}

class BasicDisabledToken
{
public:
    using IsActive = std::false_type;

    BasicDisabledToken() {}

    BasicDisabledToken(const BasicDisabledToken &) = default;
    BasicDisabledToken &operator=(const BasicDisabledToken &) = default;
    BasicDisabledToken(BasicDisabledToken &&other) noexcept = default;
    BasicDisabledToken &operator=(BasicDisabledToken &&other) noexcept = default;

    ~BasicDisabledToken() {}

    constexpr explicit operator bool() const { return false; }

    static constexpr bool isActive() { return false; }
};

class BasicEnabledToken
{
public:
    using IsActive = std::true_type;

    BasicEnabledToken() {}

    BasicEnabledToken(const BasicEnabledToken &) = default;
    BasicEnabledToken &operator=(const BasicEnabledToken &) = default;
    BasicEnabledToken(BasicEnabledToken &&other) noexcept = default;
    BasicEnabledToken &operator=(BasicEnabledToken &&other) noexcept = default;
    ~BasicEnabledToken() {}

    constexpr explicit operator bool() const { return false; }

    static constexpr bool isActive() { return false; }
};

template<typename Category, bool enabled>
class ObjectToken : public BasicDisabledToken
{
public:
    using ArgumentType = typename Category::ArgumentType;

    ObjectToken() = default;
    ObjectToken(const ObjectToken &) = delete;
    ObjectToken &operator=(const ObjectToken &) = delete;
    ObjectToken(ObjectToken &&other) noexcept = default;
    ObjectToken &operator=(ObjectToken &&other) noexcept = default;
    ~ObjectToken() = default;

    template<typename... Arguments>
    void change(ArgumentType, Arguments &&...)
    {}
};

template<typename Category>
class ObjectToken<Category, true> : public BasicEnabledToken
{
    ObjectToken(std::string_view name, std::size_t id, Category &category)
        : m_name{name}
        , m_id{id}
        , m_category{&category}
    {}

public:
    using StringType = typename Category::StringType;
    using ArgumentType = typename Category::ArgumentType;

    friend Category;

    ObjectToken() = default;

    ObjectToken(const ObjectToken &other) = delete;

    ObjectToken &operator=(const ObjectToken &other) = delete;

    ObjectToken(ObjectToken &&other) noexcept
        : m_name{std::move(other.m_name)}
        , m_id{std::exchange(other.m_id, 0)}
        , m_category{std::exchange(other.m_category, nullptr)}
    {}

    ObjectToken &operator=(ObjectToken &&other) noexcept
    {
        if (&other != this) {
            m_name = std::move(other.m_name);
            m_id = std::exchange(other.m_id, 0);
            m_category = std::exchange(other.m_category, nullptr);
        }

        return *this;
    }

    ~ObjectToken()
    {
        if (m_id)
            m_category->end('e', m_id, std::move(m_name));

        m_id = 0;
    }

    template<typename... Arguments>
    void change(ArgumentType name, Arguments &&...arguments)
    {
        if (m_id)
            m_category->tick('n', m_id, std::move(name), std::forward<Arguments>(arguments)...);
    }

private:
    StringType m_name;
    std::size_t m_id = 0;
    Category *m_category = nullptr;
};

template<typename Category, bool enabled>
class AsynchronousToken : public BasicDisabledToken
{
public:
    using ArgumentType = typename Category::ArgumentType;

    AsynchronousToken() {}

    AsynchronousToken(const AsynchronousToken &) = delete;
    AsynchronousToken &operator=(const AsynchronousToken &) = delete;
    AsynchronousToken(AsynchronousToken &&other) noexcept = default;
    AsynchronousToken &operator=(AsynchronousToken &&other) noexcept = default;

    ~AsynchronousToken() {}

    template<typename... Arguments>
    AsynchronousToken begin(ArgumentType, Arguments &&...)
    {
        return AsynchronousToken{};
    }

    template<typename... Arguments>
    void tick(Arguments &&...)
    {}

    template<typename... Arguments>
    void end(Arguments &&...)
    {}
};

template<typename TraceEvent, bool enabled>
class Category;

template<typename Category>
class AsynchronousToken<Category, true> : public BasicEnabledToken
{
    AsynchronousToken(std::string_view name, std::size_t id, Category &category)
        : m_name{name}
        , m_id{id}
        , m_category{&category}
    {}

public:
    using StringType = typename Category::StringType;
    using ArgumentType = typename Category::ArgumentType;

    friend Category;

    AsynchronousToken() = default;

    AsynchronousToken(const AsynchronousToken &) = delete;
    AsynchronousToken &operator=(const AsynchronousToken &) = delete;

    AsynchronousToken(AsynchronousToken &&other) noexcept
        : m_name{std::move(other.m_name)}
        , m_id{std::exchange(other.m_id, 0)}
        , m_category{std::exchange(other.m_category, nullptr)}
    {}

    AsynchronousToken &operator=(AsynchronousToken &&other) noexcept
    {
        if (&other != this) {
            m_name = std::move(other.m_name);
            m_id = std::exchange(other.m_id, 0);
            m_category = std::exchange(other.m_category, nullptr);
        }

        return *this;
    }

    ~AsynchronousToken() { end(); }

    template<typename... Arguments>
    AsynchronousToken begin(ArgumentType name, Arguments &&...arguments)
    {
        if (m_id)
            m_category->begin('b', m_id, std::move(name), std::forward<Arguments>(arguments)...);

        return AsynchronousToken{m_name, m_id, *m_category};
    }

    template<typename... Arguments>
    void tick(ArgumentType name, Arguments &&...arguments)
    {
        if (m_id)
            m_category->tick('n', m_id, std::move(name), std::forward<Arguments>(arguments)...);
    }

    template<typename... Arguments>
    void end(Arguments &&...arguments)
    {
        if (m_id)
            m_category->end('e', m_id, std::move(m_name), std::forward<Arguments>(arguments)...);

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
    using ArgumentsStringType = typename TraceEvent::ArgumentsStringType;
    using AsynchronousTokenType = AsynchronousToken<Category, false>;
    using ObjectTokenType = ObjectToken<Category, false>;

    Category(ArgumentType, EventQueue<TraceEvent, true> &) {}

    Category(ArgumentType, EventQueue<TraceEvent, false> &) {}

    template<typename... Arguments>
    AsynchronousTokenType beginAsynchronous(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    ObjectTokenType beginObject(ArgumentType, Arguments &&...)
    {
        return {};
    }

    static constexpr bool isActive() { return false; }
};

template<typename TraceEvent>
class Category<TraceEvent, true>
{
public:
    using IsActive = std::true_type;
    using ArgumentType = typename TraceEvent::ArgumentType;
    using ArgumentsStringType = typename TraceEvent::ArgumentsStringType;
    using StringType = typename TraceEvent::StringType;
    using AsynchronousTokenType = AsynchronousToken<Category, true>;
    using ObjectTokenType = ObjectToken<Category, true>;

    friend AsynchronousTokenType;
    friend ObjectTokenType;

    template<typename EventQueue>
    Category(ArgumentType name, EventQueue &queue)
        : m_name{std::move(name)}
        , m_eventQueue{queue}
    {
        static_assert(std::is_same_v<typename EventQueue::IsActive, std::true_type>,
                      "A active category is not possible with an inactive event queue!");
        idCounter = globalIdCounter += 1ULL << 32;
    }

    template<typename... Arguments>
    AsynchronousTokenType beginAsynchronous(ArgumentType traceName, Arguments &&...arguments)
    {
        std::size_t id = ++idCounter;

        begin('b', id, std::move(traceName), std::forward<Arguments>(arguments)...);

        return {traceName, id, *this};
    }

    ObjectTokenType beginObject(ArgumentType traceName)
    {
        std::size_t id = ++idCounter;

        if (id)
            begin('b', id, std::move(traceName));

        return {traceName, id, *this};
    }

    template<typename... Arguments>
    ObjectTokenType beginObject(ArgumentType traceName, Arguments &&...arguments)
    {
        std::size_t id = ++idCounter;

        if (id)
            begin('b', id, std::move(traceName), std::forward<Arguments>(arguments)...);

        return {traceName, id, *this};
    }

    EnabledEventQueue<TraceEvent> &eventQueue() const { return m_eventQueue; }

    std::string_view name() const { return m_name; }

    static constexpr bool isActive() { return true; }

private:
    static void appendArguments(TraceEvent &) {}

    static void appendArguments(TraceEvent &traceEvent, TracerLiteral arguments)
    {
        traceEvent.arguments = arguments;
    }

    template<typename LocalTraceEvent, typename... Arguments>
    static void appendArguments(LocalTraceEvent &traceEvent, Arguments &&...arguments)
    {
        using String = typename LocalTraceEvent::ArgumentsStringType;
        static_assert(
            !std::is_same_v<String, std::string_view>,
            R"(The arguments type of the tracing event queue is a string view. You can only provide trace token arguments as TracerLiteral (""_t).)");

        traceEvent.arguments = Internal::toArguments<String>(std::forward<Arguments>(arguments)...);
    }

    template<typename... Arguments>
    void begin(char type, std::size_t id, StringType traceName, Arguments &&...arguments)
    {
        auto &traceEvent = getTraceEvent(m_eventQueue);

        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.type = type;
        traceEvent.id = id;
        appendArguments(traceEvent, std::forward<Arguments>(arguments)...);
        traceEvent.time = Clock::now();
    }

    template<typename... Arguments>
    void tick(char type, std::size_t id, StringType traceName, Arguments &&...arguments)
    {
        auto time = Clock::now();

        auto &traceEvent = getTraceEvent(m_eventQueue);

        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.time = time;
        traceEvent.type = type;
        traceEvent.id = id;
        appendArguments(traceEvent, std::forward<Arguments>(arguments)...);
    }

    template<typename... Arguments>
    void end(char type, std::size_t id, StringType traceName, Arguments &&...arguments)
    {
        auto time = Clock::now();

        auto &traceEvent = getTraceEvent(m_eventQueue);

        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.time = time;
        traceEvent.type = type;
        traceEvent.id = id;
        appendArguments(traceEvent, std::forward<Arguments>(arguments)...);
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
template<bool enabled>
using StringViewWithStringArgumentsCategory = Category<StringViewWithStringArgumentsTraceEvent, enabled>;

template<typename Category>
class Tracer
{
public:
    using ArgumentType = typename Category::ArgumentType;

    constexpr Tracer(ArgumentType, Category &, ArgumentType) {}

    constexpr Tracer(ArgumentType, Category &) {}

    Tracer(const Tracer &) = delete;
    Tracer &operator=(const Tracer &) = delete;
    Tracer(Tracer &&other) noexcept = default;
    Tracer &operator=(Tracer &&other) noexcept = delete;
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

    Tracer(const Tracer &) = delete;
    Tracer &operator=(const Tracer &) = delete;
    Tracer(Tracer &&other) noexcept = default;
    Tracer &operator=(Tracer &&other) noexcept = delete;
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
        , m_arguments{std::move(arguments)}
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

#ifdef NANOTRACE_ENABLED
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
#else
class GlobalTracer
{
public:
    GlobalTracer(std::string_view, std::string_view, std::string_view) {}

    GlobalTracer(std::string_view, std::string_view) {}

    ~GlobalTracer() {}
};
#endif

} // namespace NanotraceHR
