// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "nanotracehr.h"

#include <utils/smallstringio.h>

#include <QCoreApplication>
#include <QImage>
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

bool hasId(char phase)
{
    switch (phase) {
    case 'b':
    case 'n':
    case 'e':
        return true;
    }

    return false;
}

template<typename Id>
unsigned int getUnsignedIntegerHash(Id id)
{
    return static_cast<unsigned int>(id & 0xFFFFFFFF);
}

unsigned int getUnsignedIntegerHash(std::thread::id id)
{
    return static_cast<unsigned int>(std::hash<std::thread::id>{}(id) & 0xFFFFFFFF);
}

constexpr bool isArgumentValid(const ArgumentsString &string)
{
    return string.isValid() && string.size();
}

template<typename TraceEvent>
void printEvent(std::ostream &out, const TraceEvent &event, qint64 processId, std::thread::id threadId)
{
    out << R"({"ph":")" << event.type << R"(","name":")" << event.name << R"(","cat":")"
        << event.category << R"(","ts":)"
        << static_cast<double>(event.time.time_since_epoch().count()) / 1000 << R"(,"pid":)"
        << getUnsignedIntegerHash(processId) << R"(,"tid":)" << getUnsignedIntegerHash(threadId);

    if (event.type == 'X')
        out << R"(,"dur":)" << static_cast<double>(event.duration.count()) / 1000;

    if (hasId(event.type))
        out << R"(,"id":)" << event.id;

    if (event.bindId) {
        out << R"(,"bind_id":)" << event.bindId;
        if (event.flow & IsFlow::Out)
            out << R"(,"flow_out":true)";
        if (event.flow & IsFlow::In)
            out << R"(,"flow_in":true)";
    }

    if constexpr (!std::is_same_v<TraceEvent, TraceEventWithoutArguments>) {
        if (isArgumentValid(event.arguments)) {
            out << R"(,"args":)" << event.arguments;
        }
    }

    out << "}";
}

void writeMetaEvent(EnabledTraceFile &file, std::string_view key, std::string_view value)
{
    auto &out = file.out;

    if (out.is_open()) {
        std::lock_guard lock{file.flushMutex};

        file.out << R"({"name":")" << key << R"(","ph":"M", "pid":)"
                 << getUnsignedIntegerHash(QCoreApplication::applicationPid()) << R"(,"tid":)"
                 << getUnsignedIntegerHash(std::this_thread::get_id()) << R"(,"args":{"name":")"
                 << value << R"("}})"
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

template<typename String>
void convertToString(String &string, const QImage &image)
{
    using namespace std::string_view_literals;
    auto dict = dictonary(keyValue("width", image.width()),
                          keyValue("height", image.height()),
                          keyValue("bytes", image.sizeInBytes()),
                          keyValue("has alpha channel", image.hasAlphaChannel()),
                          keyValue("is color", !image.isGrayscale()),
                          keyValue("pixel format",
                                   dictonary(keyValue("bits per pixel",
                                                      image.pixelFormat().bitsPerPixel()),
                                             keyValue("byte order",
                                                      [&] {
                                                          if (image.pixelFormat().byteOrder()
                                                              == QPixelFormat::BigEndian)
                                                              return "big endian"sv;
                                                          else
                                                              return "little endian"sv;
                                                      }),
                                             keyValue("premultiplied", [&] {
                                                 if (image.pixelFormat().premultiplied()
                                                     == QPixelFormat::Premultiplied)
                                                     return "premultiplied"sv;
                                                 else
                                                     return "alpha premultiplied"sv;
                                             }))));

    convertToString(string, dict);
}

template NANOTRACE_EXPORT void convertToString(ArgumentsString &string, const QImage &image);

template<typename TraceEvent>
void flushEvents(const Utils::span<TraceEvent> events, std::thread::id threadId, EnabledTraceFile &file)
{
    if (events.empty())
        return;

    std::lock_guard lock{file.flushMutex};

    auto &out = file.out;

    if (out.is_open()) {
        auto processId = QCoreApplication::applicationPid();
        for (const auto &event : events) {
            printEvent(out, event, processId, threadId);
            out << ",\n";
        }
    }
}

template void flushEvents(const Utils::span<TraceEventWithArguments> events,
                          std::thread::id threadId,
                          EnabledTraceFile &file);

template void flushEvents(const Utils::span<TraceEventWithoutArguments> events,
                          std::thread::id threadId,
                          EnabledTraceFile &file);

void openFile(EnabledTraceFile &file)
{
    if (file.out = std::ofstream{file.filePath, std::ios::trunc}; file.out.good()) {
        std::lock_guard lock{file.flushMutex};

        file.out << std::fixed << std::setprecision(3) << R"({"traceEvents": [)";
        file.out << R"({"name":"process_name","ph":"M", "pid":)"
                 << QCoreApplication::applicationPid() << R"(,"args":{"name":"QtCreator"}})"
                 << ",\n";
    }
}

void startDispatcher(EnabledTraceFile &file)
{
    auto dispatcher = [&]() {
        std::unique_lock<std::mutex> lock{file.tasksMutex};

        auto newWork = [&] { return not file.isRunning or not file.tasks.empty(); };

        struct Dispatcher
        {
            void operator()(const TaskWithArguments &task)
            {
                flushEvents(task.data, task.threadId, file);
            }
            void operator()(const TaskWithoutArguments &task)
            {
                flushEvents(task.data, task.threadId, file);
            }
            void operator()(const MetaData &task) { writeMetaEvent(file, task.key, task.value); }

            EnabledTraceFile &file;
        };

        Dispatcher dispatcher{file};

        while (file.isRunning) {
            file.condition.wait(lock, newWork);

            if (not file.isRunning)
                break;

            while (not file.tasks.empty()) {
                auto task = std::move(file.tasks.front());
                file.tasks.erase(file.tasks.begin());
                lock.unlock();
                std::visit(dispatcher, task);
                lock.lock();
            }
        }
    };

    file.thread = std::thread{dispatcher};
}

void finalizeFile(EnabledTraceFile &file)
{
    {
        std::unique_lock<std::mutex> lock{file.tasksMutex};
        file.isRunning = false;
    }
    file.condition.notify_all();
    file.thread.join();
    auto &out = file.out;

    if (out.is_open()) {
        std::lock_guard lock{file.flushMutex};

#ifdef Q_OS_WINDOWS
        out.seekp(-3, std::ios_base::cur); // removes last comma and new line
#else
        out.seekp(-2, std::ios_base::cur); // removes last comma and new line
#endif
        out << R"(],"displayTimeUnit":"ns","otherData":{"version": "Qt Creator )";
        out << QCoreApplication::applicationVersion().toStdString();
        out << R"("}})";
        out.close();
    }
}

template<typename TraceEvent>
void flushInThread(EnabledEventQueue<TraceEvent> &eventQueue)
{
    {
        std::unique_lock taskLock{eventQueue.mutex};
        std::unique_lock tasksLock{eventQueue.file.tasksMutex};

        eventQueue.file.tasks.emplace_back(std::in_place_type<typename TraceEvent::Task>,
                                           std::move(taskLock),
                                           eventQueue.currentEvents.subspan(0, eventQueue.eventsIndex),
                                           eventQueue.threadId);
    }

    eventQueue.currentEvents = eventQueue.currentEvents.data() == eventQueue.eventsOne.data()
                                   ? eventQueue.eventsTwo
                                   : eventQueue.eventsOne;
    eventQueue.eventsIndex = 0;

    eventQueue.file.condition.notify_all();
}

template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<TraceEventWithArguments> &eventQueue);
template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<TraceEventWithoutArguments> &eventQueue);

namespace Internal {
template<typename TraceEvent>
EventQueueTracker<TraceEvent> &EventQueueTracker<TraceEvent>::get()
{
    static EventQueueTracker<TraceEvent> tracker;

    return tracker;
}

template NANOTRACE_EXPORT_TEMPLATE EventQueueTracker<TraceEventWithArguments> &
EventQueueTracker<TraceEventWithArguments>::get();
template NANOTRACE_EXPORT_TEMPLATE EventQueueTracker<TraceEventWithoutArguments> &
EventQueueTracker<TraceEventWithoutArguments>::get();

} // namespace Internal

template<typename TraceEvent>
EventQueue<TraceEvent, Tracing::IsEnabled>::EventQueue(EnabledTraceFile &file)
    : file{file}
    , threadId{std::this_thread::get_id()}
{
    setEventsSpans(*eventArrayOne.get(), *eventArrayTwo.get());
    Internal::EventQueueTracker<TraceEvent>::get().addQueue(this);
    if (QThread::currentThread()) {
        auto name = getThreadName();
        if (name.size()) {
            {
                std::unique_lock taskLock{mutex};
                std::lock_guard _{this->file.tasksMutex};

                this->file.tasks.emplace_back(std::in_place_type<MetaData>,
                                              std::move(taskLock),
                                              "thread_name",
                                              name);
            }
            this->file.condition.notify_all();
        }
    }
}

template<typename TraceEvent>
EventQueue<TraceEvent, Tracing::IsEnabled>::~EventQueue()
{
    Internal::EventQueueTracker<TraceEvent>::get().removeQueue(this);

    flush();
}

template<typename TraceEvent>
void EventQueue<TraceEvent, Tracing::IsEnabled>::setEventsSpans(TraceEventsSpan eventsSpanOne,
                                                                TraceEventsSpan eventsSpanTwo)
{
    eventsOne = eventsSpanOne;
    eventsTwo = eventsSpanTwo;
    currentEvents = eventsSpanOne;
}

template<typename TraceEvent>
void EventQueue<TraceEvent, Tracing::IsEnabled>::flush()
{
    std::lock_guard _{mutex};

    if (isEnabled == IsEnabled::Yes and not isFlushed)
        flushEvents(currentEvents.subspan(0, eventsIndex), threadId, file);

    isFlushed = true;
}

template class NANOTRACE_EXPORT_TEMPLATE EventQueue<TraceEventWithArguments, Tracing::IsEnabled>;
template class NANOTRACE_EXPORT_TEMPLATE EventQueue<TraceEventWithoutArguments, Tracing::IsEnabled>;

} // namespace NanotraceHR
