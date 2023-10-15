// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nanotracehr.h"

#include <QCoreApplication>
#include <QThread>

#include <fstream>
#include <iomanip>
#include <limits>
#include <thread>

#ifdef Q_OS_UNIX
#  include <pthread.h>
#endif

namespace NanotraceHR {

namespace {

template<typename TraceEvent>
void printEvent(std::ostream &out, const TraceEvent &event, qint64 processId, std::thread::id threadId)
{
    out << R"({"ph":")" << event.type << R"(","name":")" << event.name << R"(","cat":")"
        << event.category << R"(","ts":)"
        << static_cast<double>(event.time.time_since_epoch().count()) / 1000 << R"(,"pid":)"
        << processId << R"(,"tid":)" << threadId;

    if (event.type == 'X')
        out << R"(,"dur":)" << static_cast<double>(event.duration.count()) / 1000;

    if (event.id != 0)
        out << R"(,"id":")" << event.id << R"(")";

    if (event.arguments.size())
        out << R"(,"args":)" << event.arguments;

    out << "}";
}

void writeMetaEvent(TraceFile<Tracing::IsEnabled> *file, std::string_view key, std::string_view value)
{
    std::lock_guard lock{file->fileMutex};
    auto &out = file->out;

    if (out.is_open()) {
        file->out << R"({"name":")" << key << R"(","ph":"M", "pid":)"
                  << QCoreApplication::applicationPid() << R"(,"tid":)"
                  << std::this_thread::get_id() << R"(,"args":{"name":")" << value << R"("}})"
                  << ",\n";
    }
}

std::string getThreadName()
{
    std::array<char, 200> buffer;
    buffer[0] = 0;
#ifdef Q_OS_UNIX
    auto rc = pthread_getname_np(pthread_self(), buffer.data(), buffer.size());
    if (rc != 0)
        return {};

#endif

    return buffer.data();
}

} // namespace

template<typename TraceEvent>
void flushEvents(const Utils::span<TraceEvent> events,
                 std::thread::id threadId,
                 EnabledEventQueue<TraceEvent> &eventQueue)
{
    if (events.empty())
        return;

    std::lock_guard lock{eventQueue.file->fileMutex};
    auto &out = eventQueue.file->out;

    if (out.is_open()) {
        auto processId = QCoreApplication::applicationPid();
        for (const auto &event : events) {
            printEvent(out, event, processId, threadId);
            out << ",\n";
        }
    }
}

template NANOTRACE_EXPORT void flushEvents(const Utils::span<StringViewTraceEvent> events,
                                           std::thread::id threadId,
                                           EnabledEventQueue<StringViewTraceEvent> &eventQueue);
template NANOTRACE_EXPORT void flushEvents(const Utils::span<StringTraceEvent> events,
                                           std::thread::id threadId,
                                           EnabledEventQueue<StringTraceEvent> &eventQueue);
template NANOTRACE_EXPORT void flushEvents(
    const Utils::span<StringViewWithStringArgumentsTraceEvent> events,
    std::thread::id threadId,
    EnabledEventQueue<StringViewWithStringArgumentsTraceEvent> &eventQueue);

void openFile(EnabledTraceFile &file)
{
    std::lock_guard lock{file.fileMutex};

    if (file.out = std::ofstream{file.filePath, std::ios::trunc}; file.out.good()) {
        file.out << std::fixed << std::setprecision(3) << R"({"traceEvents": [)";
        file.out << R"({"name":"process_name","ph":"M", "pid":)"
                 << QCoreApplication::applicationPid() << R"(,"args":{"name":"QtCreator"}})"
                 << ",\n";
    }
}

void finalizeFile(EnabledTraceFile &file)
{
    std::lock_guard lock{file.fileMutex};
    auto &out = file.out;

    if (out.is_open()) {
        out.seekp(-2, std::ios_base::cur); // removes last comma and new line
        out << R"(],"displayTimeUnit":"ns","otherData":{"version": "Qt Creator )";
        out << QCoreApplication::applicationVersion().toStdString();
        out << R"("}})";
        out.close();
    }
}

template<typename TraceEvent>
void flushInThread(EnabledEventQueue<TraceEvent> &eventQueue)
{
    if (eventQueue.file->processing.valid())
        eventQueue.file->processing.wait();

    auto flush = [&](const Utils::span<TraceEvent> &events, std::thread::id threadId) {
        flushEvents(events, threadId, eventQueue);
    };

    eventQueue.file->processing = std::async(std::launch::async,
                                             flush,
                                             eventQueue.currentEvents.subspan(0, eventQueue.eventsIndex),
                                             std::this_thread::get_id());
    eventQueue.currentEvents = eventQueue.currentEvents.data() == eventQueue.eventsOne.data()
                                   ? eventQueue.eventsTwo
                                   : eventQueue.eventsOne;
    eventQueue.eventsIndex = 0;
}

template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringViewTraceEvent> &eventQueue);
template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringTraceEvent> &eventQueue);
template NANOTRACE_EXPORT void flushInThread(
    EnabledEventQueue<StringViewWithStringArgumentsTraceEvent> &eventQueue);

namespace {
TraceFile<tracingStatus()> globalTraceFile{"global.json"};
thread_local EventQueueData<StringTraceEvent, 1000, tracingStatus()> globalEventQueueData{
    globalTraceFile};
thread_local EventQueue s_globalEventQueue = globalEventQueueData.createEventQueue();
} // namespace

EventQueue<StringTraceEvent, tracingStatus()> &globalEventQueue()
{
    return s_globalEventQueue;
}

template<typename TraceEvent>
EventQueue<TraceEvent, Tracing::IsEnabled>::EventQueue(EnabledTraceFile *file,
                                                       TraceEventsSpan eventsOne,
                                                       TraceEventsSpan eventsTwo)
    : file{file}
    , eventsOne{eventsOne}
    , eventsTwo{eventsTwo}
    , currentEvents{eventsOne}
{
    if (auto thread = QThread::currentThread()) {
        connection = QObject::connect(QCoreApplication::instance(),
                                      &QCoreApplication::aboutToQuit,
                                      thread,
                                      [&] { flush(); });
        auto name = getThreadName();
        if (name.size()) {
            writeMetaEvent(file, "thread_name", name);
        }
    }
}

template<typename TraceEvent>
EventQueue<TraceEvent, Tracing::IsEnabled>::~EventQueue()
{
    flush();
    if (connection)
        QObject::disconnect(connection);
}

template<typename TraceEvent>
void EventQueue<TraceEvent, Tracing::IsEnabled>::flush()
{
    std::lock_guard lock{mutex};
    if (isEnabled == IsEnabled::Yes && eventsIndex > 0) {
        flushEvents(currentEvents.subspan(0, eventsIndex), std::this_thread::get_id(), *this);
        eventsIndex = 0;
    }
}

template class EventQueue<StringViewTraceEvent, Tracing::IsEnabled>;
template class EventQueue<StringTraceEvent, Tracing::IsEnabled>;
template class EventQueue<StringViewWithStringArgumentsTraceEvent, Tracing::IsEnabled>;

} // namespace NanotraceHR
