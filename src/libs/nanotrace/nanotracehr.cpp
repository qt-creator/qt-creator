// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nanotracehr.h"

#include <QCoreApplication>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <thread>

namespace NanotraceHR {

namespace {

template<typename TraceEvent>
void printEvent(std::ostream &out, const TraceEvent &event, qint64 processId, std::thread::id threadId)
{
    out << R"({"ph":"X","name":")" << event.name << R"(","cat":")" << event.category
        << R"(","ts":")" << static_cast<double>(event.start.time_since_epoch().count()) / 1000
        << R"(","dur":")" << static_cast<double>(event.duration.count()) / 1000 << R"(","pid":")"
        << processId << R"(","tid":")" << threadId << R"(","args":)" << event.arguments << "}";
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
        out << std::flush;
    }
}

template NANOTRACE_EXPORT void flushEvents(const Utils::span<StringViewTraceEvent> events,
                                           std::thread::id threadId,
                                           EnabledEventQueue<StringViewTraceEvent> &eventQueue);
template NANOTRACE_EXPORT void flushEvents(const Utils::span<StringTraceEvent> events,
                                           std::thread::id threadId,
                                           EnabledEventQueue<StringTraceEvent> &eventQueue);

void openFile(EnabledTraceFile &file)
{
    std::lock_guard lock{file.fileMutex};

    if (file.out = std::ofstream{file.filePath, std::ios::trunc}; file.out.good())
        file.out << std::fixed << std::setprecision(3) << R"({"traceEvents": [)";
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
                                             eventQueue.currentEvents,
                                             std::this_thread::get_id());
    eventQueue.currentEvents = eventQueue.currentEvents.data() == eventQueue.eventsOne.data()
                                   ? eventQueue.eventsTwo
                                   : eventQueue.eventsOne;
    eventQueue.eventsIndex = 0;
}

template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringViewTraceEvent> &eventQueue);
template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<StringTraceEvent> &eventQueue);

namespace {
EnabledTraceFile globalTraceFile{"global.json"};
thread_local auto globalEventQueueData = makeEventQueueData<StringTraceEvent, 1000>(globalTraceFile);
thread_local EventQueue s_globalEventQueue = globalEventQueueData.createEventQueue();
} // namespace

EnabledEventQueue<StringTraceEvent> &globalEventQueue()
{
    return s_globalEventQueue;
}

} // namespace NanotraceHR
