// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "nanotraceglobals.h"

#include "staticstring.h"

#include <utils/smallstring.h>
#include <utils/span.h>
#include <utils/utility.h>

#include <QByteArrayView>
#include <QList>
#include <QMap>
#include <QStringView>
#include <QVarLengthArray>
#include <QVariant>

#include <array>
#include <atomic>
#include <chrono>
#include <exception>
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

enum class Tracing { IsDisabled, IsEnabled };

#if __cplusplus >= 202002L && __has_cpp_attribute(msvc::no_unique_address)
#  define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif __cplusplus >= 202002L && __has_cpp_attribute(no_unique_address) >= 201803L
#  define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#  define NO_UNIQUE_ADDRESS
#endif

using ArgumentsString = StaticString<3700>;

namespace Literals {
struct TracerLiteral
{
    consteval TracerLiteral(std::string_view text)
        : text{text}
    {}

    friend consteval TracerLiteral operator""_t(const char *text, size_t size);

    constexpr operator std::string_view() const { return text; }

    operator std::string() const { return std::string{text}; }

    operator Utils::SmallString() const { return text; }

private:
    std::string_view text;
};

consteval TracerLiteral operator""_t(const char *text, size_t size)
{
    return {std::string_view{text, size}};
}
} // namespace Literals

using namespace Literals;

struct IsArray
{};

inline constexpr IsArray isArray;

struct IsDictonary
{};

inline constexpr IsDictonary isDictonary;

template<typename String>
void convertToString(String &string, std::string_view text)
{
    string.append('\"');
    string.append(text);
    string.append('\"');
}

template<typename String>
void convertToString(String &string, const QImage &image);

extern template NANOTRACE_EXPORT void convertToString(ArgumentsString &string, const QImage &image);

template<typename String, std::size_t size>
void convertToString(String &string, const char (&text)[size])
{
    string.append('\"');
    string.append(std::string_view{text, size - 1});
    string.append('\"');
}

template<typename String>
void convertToString(String &string, QStringView text)
{
    string.append('\"');
    string.append(Utils::PathString{text});
    string.append('\"');
}

template<typename String>
void convertToString(String &string, const QByteArray &text)
{
    string.append('\"');
    string.append(std::string_view(text.data(), Utils::usize(text)));
    string.append('\"');
}

template<typename String>
void convertToString(String &string, bool isTrue)
{
    if (isTrue)
        string.append("true");
    else
        string.append("false");
}

template<typename String, typename Callable, typename std::enable_if_t<std::is_invocable_v<Callable>, bool> = true>
void convertToString(String &string, Callable &&callable)
{
    convertToString(string, callable());
}

template<typename String, typename Number, typename std::enable_if_t<std::is_arithmetic_v<Number>, bool> = true>
void convertToString(String &string, Number number)
{
    string.append(number);
}

template<typename String, typename Enumeration, typename std::enable_if_t<std::is_enum_v<Enumeration>, bool> = true>
void convertToString(String &string, Enumeration enumeration)
{
    string.append(Utils::to_underlying(enumeration));
}

template<typename String>
void convertToString(String &string, const QString &text)
{
    convertToString(string, QStringView{text});
}

template<typename String>
void convertToString(String &string, const QVariant &value)
{
    convertToString(string, value.toString());
}

template<typename String, typename Type>
void convertToString(String &string, const std::optional<Type> &value)
{
    if (value)
        convertToString(string, *value);
    else
        convertToString(string, "empty optional");
}

template<typename String, typename Value>
void convertArrayEntryToString(String &string, const Value &value)
{
    convertToString(string, value);
    string.append(",");
}

template<typename String, typename... Entries>
void convertArrayToString(String &string, const IsArray &, Entries &...entries)
{
    string.append('[');
    (convertArrayEntryToString(string, entries), ...);
    if (sizeof...(entries))
        string.pop_back();
    string.append(']');
}

template<typename String, typename... Arguments>
void convertToString(String &string, const std::tuple<IsArray, Arguments...> &list)
{
    std::apply([&](auto &&...entries) { convertArrayToString(string, entries...); }, list);
}

template<typename String, typename Key, typename Value>
void convertDictonaryEntryToString(String &string, const std::tuple<Key, Value> &argument)
{
    const auto &[key, value] = argument;
    convertToString(string, key);
    string.append(':');
    convertToString(string, value);
    string.append(',');
}

template<typename String, typename... Entries>
void convertDictonaryToString(String &string, const IsDictonary &, Entries &...entries)
{
    string.append('{');
    (convertDictonaryEntryToString(string, entries), ...);
    if (sizeof...(entries))
        string.pop_back();
    string.append('}');
}

template<typename String, typename... Arguments>
void convertToString(String &string, const std::tuple<IsDictonary, Arguments...> &dictonary)
{
    std::apply([&](auto &&...entries) { convertDictonaryToString(string, entries...); }, dictonary);
}

template<typename T>
struct is_container : std::false_type
{};

template<typename... Arguments>
struct is_container<std::vector<Arguments...>> : std::true_type
{};

template<typename... Arguments>
struct is_container<QList<Arguments...>> : std::true_type
{};

template<typename T, qsizetype Prealloc>
struct is_container<QVarLengthArray<T, Prealloc>> : std::true_type
{};

template<typename String, typename Container, typename std::enable_if_t<is_container<Container>::value, bool> = true>
void convertToString(String &string, const Container &values)
{
    string.append('[');

    for (const auto &value : values) {
        convertToString(string, value);
        string.append(',');
    }

    if (values.size())
        string.pop_back();

    string.append(']');
}

template<typename T>
struct is_map : std::false_type
{};

template<typename... Arguments>
struct is_map<QtPrivate::QKeyValueRange<Arguments...>> : std::true_type
{};

template<typename... Arguments>
struct is_map<std::map<Arguments...>> : std::true_type
{};

template<typename String, typename Map, typename std::enable_if_t<is_map<Map>::value, bool> = true>
void convertToString(String &string, const Map &map)
{
    string.append('{');

    for (const auto &[key, value] : map) {
        convertToString(string, key);
        string.append(':');
        convertToString(string, value);
        string.append(',');
    }

    if (map.begin() != map.end())
        string.pop_back();

    string.append('}');
}

template<typename String, typename Key, typename Value>
void convertToString(String &string, const QMap<Key, Value> &dictonary)
{
    convertToString(string, dictonary.asKeyValueRange());
}

namespace Internal {

template<typename String, typename... Arguments>
void toArguments(String &text, Arguments &&...arguments)
{
    constexpr auto argumentCount = sizeof...(Arguments);
    text.append('{');
    (convertDictonaryEntryToString(text, arguments), ...);
    if (argumentCount)
        text.pop_back();

    text.append('}');
}

inline std::string_view toArguments(std::string_view arguments)
{
    return arguments;
}

template<typename String>
void setArguments(String &eventArguments)
{
    if constexpr (std::is_same_v<String, std::string_view>)
        eventArguments = {};
    else
        eventArguments.clear();
}

template<typename String>
void setArguments(String &eventArguments, TracerLiteral arguments)
{
    eventArguments = arguments;
}

template<typename String, typename... Arguments>
[[maybe_unused]] void setArguments(String &eventArguments, Arguments &&...arguments)
{
    static_assert(
        !std::is_same_v<String, std::string_view>,
        R"(The arguments type of the tracing event queue is a string view. You can only provide trace token arguments as TracerLiteral (""_t).)");

    if constexpr (std::is_same_v<String, std::string_view>)
        eventArguments = {};
    else
        eventArguments.clear();
    Internal::toArguments(eventArguments, std::forward<Arguments>(arguments)...);
}

} // namespace Internal

template<typename Key, typename Value>
auto keyValue(const Key &key, Value &&value)
{
    if constexpr (std::is_lvalue_reference_v<Value>)
        return std::tuple<const Key &, const Value &>(key, value);
    else
        return std::tuple<const Key &, std::decay_t<Value>>(key, value);
}

template<typename... Entries>
auto dictonary(Entries &&...entries)
{
    return std::make_tuple(isDictonary, std::forward<Entries>(entries)...);
}

template<typename... Entries>
auto array(Entries &&...entries)
{
    return std::make_tuple(isArray, std::forward<Entries>(entries)...);
}

enum class IsFlow : char { No = 0, Out = 1 << 0, In = 1 << 1, InOut = In | Out };

inline bool operator&(IsFlow first, IsFlow second)
{
    return static_cast<std::underlying_type_t<IsFlow>>(first)
           & static_cast<std::underlying_type_t<IsFlow>>(second);
}

template<typename String, typename ArgumentsString>
struct alignas(4096) TraceEvent
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
    TimePoint time;
    Duration duration;
    std::size_t id = 0;
    std::size_t bindId = 0;
    IsFlow flow = IsFlow::No;
    char type = ' ';
    ArgumentsString arguments;
};

using StringViewTraceEvent = TraceEvent<std::string_view, std::string_view>;
using StringViewWithStringArgumentsTraceEvent = TraceEvent<std::string_view, ArgumentsString>;
using StringTraceEvent = TraceEvent<Utils::SmallString, ArgumentsString>;

enum class IsEnabled { No, Yes };

template<typename TraceEvent, Tracing isEnabled>
class EventQueue;

template<typename TraceEvent>
using EnabledEventQueue = EventQueue<TraceEvent, Tracing::IsEnabled>;

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

template<Tracing isEnabled>
class TraceFile;

using EnabledTraceFile = TraceFile<Tracing::IsEnabled>;

NANOTRACE_EXPORT void openFile(EnabledTraceFile &file);
NANOTRACE_EXPORT void finalizeFile(EnabledTraceFile &file);

template<Tracing isEnabled>
class TraceFile
{
public:
    using IsActive = std::false_type;

    TraceFile(std::string_view) {}
};

template<>
class TraceFile<Tracing::IsEnabled>
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

template<typename TraceEvent, Tracing isEnabled>
class EventQueue
{
public:
    using IsActive = std::false_type;

    template<typename TraceFile> EventQueue(TraceFile &) {}
};

namespace Internal {

template<typename TraceEvent>
class EventQueueTracker
{
    using Queue = EventQueue<TraceEvent, Tracing::IsEnabled>;

public:
    EventQueueTracker()
    {
        terminateHandler = std::get_terminate();

        std::set_terminate([]() { EventQueueTracker::get().terminate(); });
    }

    EventQueueTracker(const EventQueueTracker &) = delete;
    EventQueueTracker(EventQueueTracker &&) = delete;
    EventQueueTracker &operator=(const EventQueueTracker &) = delete;
    EventQueueTracker &operator=(EventQueueTracker &&) = delete;

    ~EventQueueTracker()
    {
        std::lock_guard lock{mutex};

        for (auto queue : queues)
            queue->flush();
    }

    void addQueue(Queue *queue)
    {
        std::lock_guard lock{mutex};
        queues.push_back(queue);
    }

    void removeQueue(Queue *queue)
    {
        std::lock_guard lock{mutex};
        queues.erase(std::remove(queues.begin(), queues.end(), queue), queues.end());
    }

    static EventQueueTracker &get()
    {
        static EventQueueTracker tracker;

        return tracker;
    }

private:
    void terminate()
    {
        flushAll();
        if (terminateHandler)
            terminateHandler();
    }

    void flushAll()
    {
        std::lock_guard lock{mutex};

        for (auto queue : queues)
            queue->flush();
    }

private:
    std::mutex mutex;
    std::vector<Queue *> queues;
    std::terminate_handler terminateHandler = nullptr;
};
} // namespace Internal

template<typename TraceEvent>
class EventQueue<TraceEvent, Tracing::IsEnabled>
{
    using TraceEventsSpan = Utils::span<TraceEvent>;
    using TraceEvents = std::array<TraceEvent, 1000>;

public:
    using IsActive = std::true_type;

    EventQueue(EnabledTraceFile &file);

    ~EventQueue();

    void setEventsSpans(TraceEventsSpan eventsOne, TraceEventsSpan eventsTwo);

    void flush();

    EventQueue(const EventQueue &) = delete;
    EventQueue(EventQueue &&) = delete;
    EventQueue &operator=(const EventQueue &) = delete;
    EventQueue &operator=(EventQueue &&) = delete;

    EnabledTraceFile &file;
    std::unique_ptr<TraceEvents> eventArrayOne = std::make_unique<TraceEvents>();
    std::unique_ptr<TraceEvents> eventArrayTwo = std::make_unique<TraceEvents>();
    TraceEventsSpan eventsOne;
    TraceEventsSpan eventsTwo;
    TraceEventsSpan currentEvents;
    std::size_t eventsIndex = 0;
    IsEnabled isEnabled = IsEnabled::Yes;
    std::mutex mutex;
    std::thread::id threadId;
};

template<Tracing isEnabled>
using StringViewEventQueue = EventQueue<StringViewTraceEvent, isEnabled>;
template<Tracing isEnabled>
using StringViewWithStringArgumentsEventQueue = EventQueue<StringViewWithStringArgumentsTraceEvent, isEnabled>;
template<Tracing isEnabled>
using StringEventQueue = EventQueue<StringTraceEvent, isEnabled>;

extern template class NANOTRACE_EXPORT_EXTERN_TEMPLATE EventQueue<StringViewTraceEvent, Tracing::IsEnabled>;
extern template class NANOTRACE_EXPORT_EXTERN_TEMPLATE EventQueue<StringTraceEvent, Tracing::IsEnabled>;
extern template class NANOTRACE_EXPORT_EXTERN_TEMPLATE
    EventQueue<StringViewWithStringArgumentsTraceEvent, Tracing::IsEnabled>;

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

template<typename Category, typename IsActive>
class Tracer;

template<typename Category, Tracing isEnabled>
class Token : public BasicDisabledToken
{
public:
    using ArgumentType = typename Category::ArgumentType;
    using FlowTokenType = typename Category::FlowTokenType;
    using AsynchronousTokenType = typename Category::AsynchronousTokenType;
    using TracerType = typename Category::TracerType;

    Token() {}

    ~Token() {}

    template<typename... Arguments>
    [[nodiscard]] AsynchronousTokenType beginAsynchronous(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousTokenType, FlowTokenType> beginAsynchronousWithFlow(
        ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] TracerType beginDuration(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<TracerType, FlowTokenType> beginDurationWithFlow(ArgumentType,
                                                                             Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    void tick(ArgumentType, Arguments &&...)
    {}
};

template<typename Category>
class Token<Category, Tracing::IsEnabled> : public BasicEnabledToken

{
    using CategoryFunctionPointer = typename Category::CategoryFunctionPointer;

    Token(std::size_t id, CategoryFunctionPointer category)
        : m_id{id}
        , m_category{category}
    {}

    using PrivateTag = typename Category::PrivateTag;

public:
    using ArgumentType = typename Category::ArgumentType;
    using FlowTokenType = typename Category::FlowTokenType;
    using AsynchronousTokenType = typename Category::AsynchronousTokenType;
    using TracerType = typename Category::TracerType;

    Token(PrivateTag, std::size_t id, CategoryFunctionPointer category)
        : Token{id, category}
    {}

    friend TracerType;
    friend AsynchronousTokenType;

    ~Token() {}

    template<typename... Arguments>
    [[nodiscard]] AsynchronousTokenType beginAsynchronous(ArgumentType name, Arguments &&...arguments)
    {
        if (m_id)
            m_category().begin('b', m_id, name, 0, IsFlow::No, std::forward<Arguments>(arguments)...);

        return {std::move(name), m_id, m_category};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousTokenType, FlowTokenType> beginAsynchronousWithFlow(
        ArgumentType name, Arguments &&...arguments)
    {
        std::size_t bindId = 0;

        if (m_id) {
            auto &category = m_category();
            bindId = category.createBindId();
            category.begin('b', m_id, name, bindId, IsFlow::Out, std::forward<Arguments>(arguments)...);
        }

        return {std::piecewise_construct,
                std::forward_as_tuple(std::move(name), m_id, m_category),
                std::forward_as_tuple(std::move(name), bindId, m_category)};
    }

    template<typename... Arguments>
    [[nodiscard]] TracerType beginDuration(ArgumentType traceName, Arguments &&...arguments)
    {
        return {traceName, m_category, std::forward<Arguments>(arguments)...};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<TracerType, FlowTokenType> beginDurationWithFlow(ArgumentType traceName,
                                                                             Arguments &&...arguments)
    {
        std::size_t bindId = m_category().createBindId();

        return {std::piecewise_construct,
                std::forward_as_tuple(PrivateTag{},
                                      bindId,
                                      IsFlow::Out,
                                      traceName,
                                      m_category,
                                      std::forward<Arguments>(arguments)...),
                std::forward_as_tuple(PrivateTag{}, traceName, bindId, m_category)};
    }

    template<typename... Arguments>
    void tick(ArgumentType name, Arguments &&...arguments)
    {
        m_category().begin('i', 0, name, 0, IsFlow::No, std::forward<Arguments>(arguments)...);
    }

private:
    std::size_t m_id = 0;
    CategoryFunctionPointer m_category = nullptr;
};
template<typename TraceEvent, Tracing isEnabled>
class Category;

template<Tracing isEnabled>
using StringViewCategory = Category<StringViewTraceEvent, isEnabled>;
template<Tracing isEnabled>
using StringCategory = Category<StringTraceEvent, isEnabled>;
template<Tracing isEnabled>
using StringViewWithStringArgumentsCategory = Category<StringViewWithStringArgumentsTraceEvent, isEnabled>;

using DisabledToken = Token<StringViewCategory<Tracing::IsDisabled>, Tracing::IsDisabled>;

template<typename Category, Tracing isEnabled>
class AsynchronousToken : public BasicDisabledToken
{
public:
    using ArgumentType = typename Category::ArgumentType;
    using FlowTokenType = typename Category::FlowTokenType;
    using TokenType = typename Category::TokenType;

    AsynchronousToken() {}

    AsynchronousToken(const AsynchronousToken &) = delete;
    AsynchronousToken &operator=(const AsynchronousToken &) = delete;
    AsynchronousToken(AsynchronousToken &&other) noexcept = default;
    AsynchronousToken &operator=(AsynchronousToken &&other) noexcept = default;

    ~AsynchronousToken() {}

    [[nodiscard]] TokenType createToken() { return {}; }

    template<typename... Arguments>
    [[nodiscard]] AsynchronousToken begin(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] AsynchronousToken begin(const FlowTokenType &, ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousToken, FlowTokenType> beginWithFlow(ArgumentType,
                                                                            Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    void tick(ArgumentType, Arguments &&...)
    {}

    template<typename... Arguments>
    void tick(const FlowTokenType &, ArgumentType, Arguments &&...)
    {}

    template<typename... Arguments>
    FlowTokenType tickWithFlow(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    void end(Arguments &&...)
    {}

    static constexpr bool categoryIsActive() { return Category::isActive(); }
};

using DisabledAsynchronousToken = AsynchronousToken<StringViewCategory<Tracing::IsDisabled>,
                                                    Tracing::IsDisabled>;

template<typename Category, Tracing isEnabled>
class FlowToken;

template<typename Category>
class AsynchronousToken<Category, Tracing::IsEnabled> : public BasicEnabledToken
{
    using CategoryFunctionPointer = typename Category::CategoryFunctionPointer;

    AsynchronousToken(std::string_view name, std::size_t id, CategoryFunctionPointer category)
        : m_name{name}
        , m_id{id}
        , m_category{category}
    {}

    using PrivateTag = typename Category::PrivateTag;

public:
    using StringType = typename Category::StringType;
    using ArgumentType = typename Category::ArgumentType;
    using FlowTokenType = typename Category::FlowTokenType;
    using TokenType = typename Category::TokenType;

    friend Category;
    friend FlowTokenType;
    friend TokenType;

    AsynchronousToken(PrivateTag, std::string_view name, std::size_t id, CategoryFunctionPointer category)
        : AsynchronousToken{name, id, category}
    {}

    AsynchronousToken() {}

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

    [[nodiscard]] TokenType createToken() { return {m_id, m_category}; }

    template<typename... Arguments>
    [[nodiscard]] AsynchronousToken begin(ArgumentType name, Arguments &&...arguments)
    {
        if (m_id)
            m_category().begin('b', m_id, name, 0, IsFlow::No, std::forward<Arguments>(arguments)...);

        return AsynchronousToken{std::move(name), m_id, m_category};
    }

    template<typename... Arguments>
    [[nodiscard]] AsynchronousToken begin(const FlowTokenType &flowToken,
                                          ArgumentType name,
                                          Arguments &&...arguments)
    {
        if (m_id)
            m_category().begin('b',
                               m_id,
                               name,
                               flowToken.bindId(),
                               IsFlow::In,
                               std::forward<Arguments>(arguments)...);

        return AsynchronousToken{std::move(name), m_id, m_category};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousToken, FlowTokenType> beginWithFlow(ArgumentType name,
                                                                            Arguments &&...arguments)
    {
        std::size_t bindId = 0;

        if (m_id) {
            auto &category = m_category();
            bindId = category.createBindId();
            category.begin('b', m_id, name, bindId, IsFlow::Out, std::forward<Arguments>(arguments)...);
        }

        return {std::piecewise_construct,
                std::forward_as_tuple(PrivateTag{}, std::move(name), m_id, m_category),
                std::forward_as_tuple(PrivateTag{}, std::move(name), bindId, m_category)};
    }

    template<typename... Arguments>
    void tick(ArgumentType name, Arguments &&...arguments)
    {
        if (m_id) {
            m_category().tick(
                'n', m_id, std::move(name), 0, IsFlow::No, std::forward<Arguments>(arguments)...);
        }
    }

    template<typename... Arguments>
    void tick(const FlowTokenType &flowToken, ArgumentType name, Arguments &&...arguments)
    {
        if (m_id) {
            m_category().tick('n',
                              m_id,
                              std::move(name),
                              flowToken.bindId(),
                              IsFlow::In,
                              std::forward<Arguments>(arguments)...);
        }
    }

    template<typename... Arguments>
    FlowTokenType tickWithFlow(ArgumentType name, Arguments &&...arguments)
    {
        std::size_t bindId = 0;

        if (m_id) {
            auto &category = m_category();
            bindId = category.createBindId();
            category.tick('n', m_id, name, bindId, IsFlow::Out, std::forward<Arguments>(arguments)...);
        }

        return {PrivateTag{}, std::move(name), bindId, m_category};
    }

    template<typename... Arguments>
    void end(Arguments &&...arguments)
    {
        if (m_id && m_name.size())
            m_category().end('e', m_id, std::move(m_name), std::forward<Arguments>(arguments)...);

        m_id = 0;
    }

    static constexpr bool categoryIsActive() { return Category::isActive(); }

private:
    StringType m_name;
    std::size_t m_id = 0;
    CategoryFunctionPointer m_category = nullptr;
};

template<typename Category, Tracing isEnabled>
class FlowToken : public BasicDisabledToken
{
public:
    FlowToken() = default;
    using AsynchronousTokenType = typename Category::AsynchronousTokenType;
    using ArgumentType = typename Category::ArgumentType;
    using TracerType = typename Category::TracerType;
    using TokenType = typename Category::TokenType;

    friend TracerType;
    friend AsynchronousTokenType;
    friend TokenType;
    friend Category;

    FlowToken(const FlowToken &) = default;
    FlowToken &operator=(const FlowToken &) = default;
    FlowToken(FlowToken &&other) noexcept = default;
    FlowToken &operator=(FlowToken &&other) noexcept = default;

    ~FlowToken() {}

    template<typename... Arguments>
    [[nodiscard]] AsynchronousTokenType beginAsynchronous(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousTokenType, FlowToken> beginAsynchronousWithFlow(ArgumentType,
                                                                                        Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    void connectTo(const AsynchronousTokenType &, Arguments &&...)
    {}

    template<typename... Arguments>
    [[nodiscard]] TracerType beginDuration(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<TracerType, FlowToken> beginDurationWithFlow(ArgumentType, Arguments &&...)
    {
        return std::pair<TracerType, FlowToken>();
    }

    template<typename... Arguments>
    void tick(ArgumentType, Arguments &&...)
    {}
};

template<typename Category>
class FlowToken<Category, Tracing::IsEnabled> : public BasicDisabledToken
{
    using PrivateTag = typename Category::PrivateTag;
    using CategoryFunctionPointer = typename Category::CategoryFunctionPointer;

public:
    FlowToken() = default;

    FlowToken(PrivateTag, std::string_view name, std::size_t bindId, CategoryFunctionPointer category)
        : m_name{name}
        , m_bindId{bindId}
        , m_category{category}
    {}

    using StringType = typename Category::StringType;
    using AsynchronousTokenType = typename Category::AsynchronousTokenType;
    using ArgumentType = typename Category::ArgumentType;
    using TracerType = typename Category::TracerType;
    using TokenType = typename Category::TokenType;

    friend AsynchronousTokenType;
    friend TokenType;
    friend TracerType;
    friend Category;

    FlowToken(const FlowToken &) = default;
    FlowToken &operator=(const FlowToken &) = default;
    FlowToken(FlowToken &&other) noexcept = default;
    FlowToken &operator=(FlowToken &&other) noexcept = default;

    ~FlowToken() {}

    template<typename... Arguments>
    [[nodiscard]] AsynchronousTokenType beginAsynchronous(ArgumentType name, Arguments &&...arguments)
    {
        std::size_t id = 0;

        if (m_bindId) {
            auto &category = m_category();
            id = category.createId();
            category.begin('b', id, name, m_bindId, IsFlow::In, std::forward<Arguments>(arguments)...);
        }

        return {std::move(name), id, m_category};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousTokenType, FlowToken> beginAsynchronousWithFlow(
        ArgumentType name, Arguments &&...arguments)
    {
        std::size_t id = 0;
        std::size_t bindId = 0;

        if (m_bindId) {
            auto &category = m_category();
            id = category.createId();
            bindId = category.createBindId();
            category.begin('b', id, name, bindId, IsFlow::InOut, std::forward<Arguments>(arguments)...);
        }

        return {std::piecewise_construct,
                std::forward_as_tuple(PrivateTag{}, std::move(name), id, m_category),
                std::forward_as_tuple(PrivateTag{}, std::move(name), bindId, m_category)};
    }

    template<typename... Arguments>
    void connectTo(const AsynchronousTokenType &token, Arguments &&...arguments)
    {
        if (m_bindId && token.m_id) {
            m_category().begin('b',
                               token.m_id,
                               token.m_name,
                               m_bindId,
                               IsFlow::In,
                               std::forward<Arguments>(arguments)...);
        }
    }

    template<typename... Arguments>
    [[nodiscard]] TracerType beginDuration(ArgumentType traceName, Arguments &&...arguments)
    {
        return {m_bindId, IsFlow::In, traceName, m_category, std::forward<Arguments>(arguments)...};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<TracerType, FlowToken> beginDurationWithFlow(ArgumentType traceName,
                                                                         Arguments &&...arguments)
    {
        return {std::piecewise_construct,
                std::forward_as_tuple(PrivateTag{},
                                      m_bindId,
                                      IsFlow::InOut,
                                      traceName,
                                      m_category,
                                      std::forward<Arguments>(arguments)...),
                std::forward_as_tuple(PrivateTag{}, traceName, m_bindId, m_category)};
    }

    template<typename... Arguments>
    void tick(ArgumentType name, Arguments &&...arguments)
    {
        m_category().tick('i', 0, name, m_bindId, IsFlow::In, std::forward<Arguments>(arguments)...);
    }

    std::size_t bindId() const { return m_bindId; }

private:
    StringType m_name;
    std::size_t m_bindId = 0;
    CategoryFunctionPointer m_category = nullptr;
};

template<typename TraceEvent, Tracing isEnabled>
class Category
{
public:
    using IsActive = std::false_type;
    using ArgumentType = typename TraceEvent::ArgumentType;
    using ArgumentsStringType = typename TraceEvent::ArgumentsStringType;
    using AsynchronousTokenType = AsynchronousToken<Category, Tracing::IsDisabled>;
    using FlowTokenType = FlowToken<Category, Tracing::IsDisabled>;
    using TracerType = Tracer<Category, std::false_type>;
    using TokenType = Token<Category, Tracing::IsDisabled>;
    using CategoryFunctionPointer = Category &(*) ();

    Category(ArgumentType, EventQueue<TraceEvent, Tracing::IsDisabled> &, CategoryFunctionPointer)
    {}

    Category(ArgumentType, EventQueue<TraceEvent, Tracing::IsEnabled> &, CategoryFunctionPointer) {}

    template<typename... Arguments>
    [[nodiscard]] AsynchronousTokenType beginAsynchronous(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousTokenType, FlowTokenType> beginAsynchronousWithFlow(
        ArgumentType, Arguments &&...)
    {}

    template<typename... Arguments>
    [[nodiscard]] TracerType beginDuration(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<TracerType, FlowTokenType> beginDurationWithFlow(ArgumentType,
                                                                             Arguments &&...)
    {
        return std::pair<TracerType, FlowTokenType>();
    }

    template<typename... Arguments>
    void threadEvent(ArgumentType, Arguments &&...)
    {}

    static constexpr bool isActive() { return false; }
};

template<typename TraceEvent>
class Category<TraceEvent, Tracing::IsEnabled>
{
    class PrivateTag
    {};

public:
    using IsActive = std::true_type;
    using ArgumentType = typename TraceEvent::ArgumentType;
    using ArgumentsStringType = typename TraceEvent::ArgumentsStringType;
    using StringType = typename TraceEvent::StringType;
    using AsynchronousTokenType = AsynchronousToken<Category, Tracing::IsEnabled>;
    using FlowTokenType = FlowToken<Category, Tracing::IsEnabled>;
    using TracerType = Tracer<Category, std::true_type>;
    using TokenType = Token<Category, Tracing::IsEnabled>;
    using CategoryFunctionPointer = Category &(*) ();

    friend AsynchronousTokenType;
    friend TokenType;
    friend FlowTokenType;
    friend TracerType;

    template<typename EventQueue>
    Category(ArgumentType name, EventQueue &queue, CategoryFunctionPointer self)
        : m_name{std::move(name)}
        , m_eventQueue{queue}
        , m_self{self}
    {
        static_assert(std::is_same_v<typename EventQueue::IsActive, std::true_type>,
                      "A active category is not possible with an inactive event queue!");
        m_idCounter = m_globalIdCounter += 1ULL << 32;
        m_bindIdCounter = m_globalBindIdCounter += 1ULL << 32;
    }

    Category(const Category &) = delete;
    Category &operator=(const Category &) = delete;

    template<typename... Arguments>
    [[nodiscard]] AsynchronousTokenType beginAsynchronous(ArgumentType traceName,
                                                          Arguments &&...arguments)
    {
        std::size_t id = createId();

        begin('b', id, std::move(traceName), 0, IsFlow::No, std::forward<Arguments>(arguments)...);

        return {traceName, id, m_self};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<AsynchronousTokenType, FlowTokenType> beginAsynchronousWithFlow(
        ArgumentType traceName, Arguments &&...arguments)
    {
        std::size_t id = createId();
        std::size_t bindId = createBindId();

        begin('b', id, std::move(traceName), bindId, IsFlow::Out, std::forward<Arguments>(arguments)...);

        return {std::piecewise_construct,
                std::forward_as_tuple(PrivateTag{}, traceName, id, m_self),
                std::forward_as_tuple(PrivateTag{}, traceName, bindId, m_self)};
    }

    template<typename... Arguments>
    [[nodiscard]] TracerType beginDuration(ArgumentType traceName, Arguments &&...arguments)
    {
        return {traceName, m_self, std::forward<Arguments>(arguments)...};
    }

    template<typename... Arguments>
    [[nodiscard]] std::pair<TracerType, FlowTokenType> beginDurationWithFlow(ArgumentType traceName,
                                                                             Arguments &&...arguments)
    {
        std::size_t bindId = createBindId();

        return {std::piecewise_construct,
                std::forward_as_tuple(PrivateTag{},
                                      bindId,
                                      IsFlow::Out,
                                      traceName,
                                      m_self,
                                      std::forward<Arguments>(arguments)...),
                std::forward_as_tuple(PrivateTag{}, traceName, bindId, m_self)};
    }

    template<typename... Arguments>
    void threadEvent(ArgumentType traceName, Arguments &&...arguments)
    {
        if (isEnabled == IsEnabled::No)
            return;

        auto &traceEvent = getTraceEvent(m_eventQueue);

        traceEvent.time = Clock::now();
        traceEvent.name = std::move(traceName);
        traceEvent.category = traceName;
        traceEvent.type = 'i';
        traceEvent.id = 0;
        traceEvent.bindId = 0;
        traceEvent.flow = IsFlow::No;
        Internal::setArguments(traceEvent.arguments, std::forward<Arguments>(arguments)...);
    }

    EnabledEventQueue<TraceEvent> &eventQueue() const { return m_eventQueue; }

    std::string_view name() const { return m_name; }

    static constexpr bool isActive() { return true; }

    std::size_t createBindId() { return ++m_bindIdCounter; }

    std::size_t createId() { return ++m_idCounter; }

public:
    IsEnabled isEnabled = IsEnabled::Yes;

private:
    template<typename... Arguments>
    void begin(char type,
               std::size_t id,
               StringType traceName,
               std::size_t bindId,
               IsFlow flow,
               Arguments &&...arguments)
    {
        if (isEnabled == IsEnabled::No)
            return;

        auto &traceEvent = getTraceEvent(m_eventQueue);

        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.type = type;
        traceEvent.id = id;
        traceEvent.bindId = bindId;
        traceEvent.flow = flow;
        Internal::setArguments(traceEvent.arguments, std::forward<Arguments>(arguments)...);
        traceEvent.time = Clock::now();
    }

    template<typename... Arguments>
    void tick(char type,
              std::size_t id,
              StringType traceName,
              std::size_t bindId,
              IsFlow flow,
              Arguments &&...arguments)
    {
        if (isEnabled == IsEnabled::No)
            return;

        auto time = Clock::now();

        auto &traceEvent = getTraceEvent(m_eventQueue);

        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.time = time;
        traceEvent.type = type;
        traceEvent.id = id;
        traceEvent.bindId = bindId;
        traceEvent.flow = flow;
        Internal::setArguments(traceEvent.arguments, std::forward<Arguments>(arguments)...);
    }

    template<typename... Arguments>
    void end(char type, std::size_t id, StringType traceName, Arguments &&...arguments)
    {
        if (isEnabled == IsEnabled::No)
            return;

        auto time = Clock::now();

        auto &traceEvent = getTraceEvent(m_eventQueue);

        traceEvent.name = std::move(traceName);
        traceEvent.category = m_name;
        traceEvent.time = time;
        traceEvent.type = type;
        traceEvent.id = id;
        traceEvent.bindId = 0;
        traceEvent.flow = IsFlow::No;
        Internal::setArguments(traceEvent.arguments, std::forward<Arguments>(arguments)...);
    }

    CategoryFunctionPointer self() { return m_self; }

private:
    StringType m_name;
    EnabledEventQueue<TraceEvent> &m_eventQueue;
    inline static std::atomic<std::size_t> m_globalIdCounter;
    std::size_t m_idCounter;
    inline static std::atomic<std::size_t> m_globalBindIdCounter;
    std::size_t m_bindIdCounter;
    CategoryFunctionPointer m_self;
};

template<Tracing isEnabled>
using StringViewCategory = Category<StringViewTraceEvent, isEnabled>;
template<Tracing isEnabled>
using StringCategory = Category<StringTraceEvent, isEnabled>;
template<Tracing isEnabled>
using StringViewWithStringArgumentsCategory = Category<StringViewWithStringArgumentsTraceEvent, isEnabled>;

template<typename Category, typename IsActive>
class Tracer
{
public:
    Tracer() = default;
    using ArgumentType = typename Category::ArgumentType;
    using TokenType = typename Category::TokenType;
    using FlowTokenType = typename Category::FlowTokenType;

    friend TokenType;
    friend FlowTokenType;
    friend Category;

    template<typename... Arguments>
    [[nodiscard]] Tracer(ArgumentType, Category &, Arguments &&...)
    {}

    Tracer(const Tracer &) = delete;
    Tracer &operator=(const Tracer &) = delete;
    Tracer(Tracer &&other) noexcept = default;
    Tracer &operator=(Tracer &&other) noexcept = delete;

    TokenType createToken() { return {}; }

    template<typename... Arguments>
    Tracer beginDuration(ArgumentType, Arguments &&...)
    {
        return {};
    }

    template<typename... Arguments>
    void tick(ArgumentType, Arguments &&...)
    {}

    template<typename... Arguments>
    void end(Arguments &&...)
    {}

    ~Tracer() {}
};

template<typename Category>
class Tracer<Category, std::true_type>
{
    using ArgumentType = typename Category::ArgumentType;
    using StringType = typename Category::StringType;
    using ArgumentsStringType = typename Category::ArgumentsStringType;
    using TokenType = typename Category::TokenType;
    using FlowTokenType = typename Category::FlowTokenType;
    using PrivateTag = typename Category::PrivateTag;
    using CategoryFunctionPointer = typename Category::CategoryFunctionPointer;

    friend FlowTokenType;
    friend TokenType;
    friend Category;

    template<typename... Arguments>
    [[nodiscard]] Tracer(std::size_t bindId,
                         IsFlow flow,
                         ArgumentType name,
                         CategoryFunctionPointer category,
                         Arguments &&...arguments)
        : m_name{name}
        , m_bindId{bindId}
        , flow{flow}
        , m_category{category}
    {
        if (category().isEnabled == IsEnabled::Yes)
            sendBeginTrace(std::forward<Arguments>(arguments)...);
    }

public:
    template<typename... Arguments>
    [[nodiscard]] Tracer(PrivateTag,
                         std::size_t bindId,
                         IsFlow flow,
                         ArgumentType name,
                         CategoryFunctionPointer category,
                         Arguments &&...arguments)
        : Tracer{bindId, flow, std::move(name), category, std::forward<Arguments>(arguments)...}
    {}

    template<typename... Arguments>
    [[nodiscard]] Tracer(ArgumentType name, CategoryFunctionPointer category, Arguments &&...arguments)
        : m_name{name}
        , m_category{category}
    {
        if (category().isEnabled == IsEnabled::Yes)
            sendBeginTrace(std::forward<Arguments>(arguments)...);
    }

    template<typename... Arguments>
    [[nodiscard]] Tracer(ArgumentType name, Category &category, Arguments &&...arguments)
        : Tracer(std::move(name), category.self(), std::forward<Arguments>(arguments)...)
    {}

    Tracer(const Tracer &) = delete;
    Tracer &operator=(const Tracer &) = delete;
    Tracer(Tracer &&other) noexcept = delete;
    Tracer &operator=(Tracer &&other) noexcept = delete;

    TokenType createToken() { return {0, m_category}; }

    ~Tracer() { sendEndTrace(); }

    template<typename... Arguments>
    Tracer beginDuration(ArgumentType name, Arguments &&...arguments)
    {
        return {std::move(name), m_category, std::forward<Arguments>(arguments)...};
    }

    template<typename... Arguments>
    void tick(ArgumentType name, Arguments &&...arguments)
    {
        m_category().begin('i', 0, name, 0, IsFlow::No, std::forward<Arguments>(arguments)...);
    }

    template<typename... Arguments>
    void end(Arguments &&...arguments)
    {
        sendEndTrace(std::forward<Arguments>(arguments)...);
        m_name = {};
    }

private:
    template<typename... Arguments>
    void sendBeginTrace(Arguments &&...arguments)
    {
        auto &category = m_category();
        if (category.isEnabled == IsEnabled::Yes) {
            auto &traceEvent = getTraceEvent(category.eventQueue());
            traceEvent.name = m_name;
            traceEvent.category = category.name();
            traceEvent.bindId = m_bindId;
            traceEvent.flow = flow;
            traceEvent.type = 'B';
            Internal::setArguments<ArgumentsStringType>(traceEvent.arguments,
                                                        std::forward<Arguments>(arguments)...);
            traceEvent.time = Clock::now();
        }
    }

    template<typename... Arguments>
    void sendEndTrace(Arguments &&...arguments)
    {
        if (m_name.size()) {
            auto &category = m_category();
            if (category.isEnabled == IsEnabled::Yes) {
                auto end = Clock::now();
                auto &traceEvent = getTraceEvent(category.eventQueue());
                traceEvent.name = std::move(m_name);
                traceEvent.category = category.name();
                traceEvent.time = end;
                traceEvent.bindId = m_bindId;
                traceEvent.flow = flow;
                traceEvent.type = 'E';
                Internal::setArguments<ArgumentsStringType>(traceEvent.arguments,
                                                            std::forward<Arguments>(arguments)...);
            }
        }
    }

private:
    StringType m_name;
    std::size_t m_bindId = 0;
    IsFlow flow = IsFlow::No;
    CategoryFunctionPointer m_category;
};

template<typename Category, typename... Arguments>
Tracer(typename Category::ArgumentType name, Category &category, Arguments &&...)
    -> Tracer<Category, typename Category::IsActive>;

} // namespace NanotraceHR
