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

template<typename String>
constexpr bool isArgumentValid(const String &string)
{
    return string.size();
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

void writeMetaEvent(TraceFile<Tracing::IsEnabled> &file, std::string_view key, std::string_view value)
{
    std::lock_guard lock{file.fileMutex};
    auto &out = file.out;

    if (out.is_open()) {
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
void flushEvents(const Utils::span<TraceEvent> events,
                 std::thread::id threadId,
                 std::shared_ptr<EnabledTraceFile> file)
{
    if (events.empty())
        return;

    std::lock_guard lock{file->fileMutex};
    auto &out = file->out;

    if (out.is_open()) {
        auto processId = QCoreApplication::applicationPid();
        for (const auto &event : events) {
            printEvent(out, event, processId, threadId);
            out << ",\n";
        }
    }
}

template NANOTRACE_EXPORT void flushEvents(const Utils::span<TraceEventWithArguments> events,
                                           std::thread::id threadId,
                                           std::shared_ptr<EnabledTraceFile> file);

template NANOTRACE_EXPORT void flushEvents(const Utils::span<TraceEventWithoutArguments> events,
                                           std::thread::id threadId,
                                           std::shared_ptr<EnabledTraceFile> file);

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

    auto flush = [](const Utils::span<TraceEvent> &events,
                    std::thread::id threadId,
                    std::shared_ptr<EnabledTraceFile> file) { flushEvents(events, threadId, file); };

    eventQueue.file->processing = std::async(std::launch::async,
                                             flush,
                                             eventQueue.currentEvents.subspan(0, eventQueue.eventsIndex),
                                             eventQueue.threadId,
                                             eventQueue.file);
    eventQueue.currentEvents = eventQueue.currentEvents.data() == eventQueue.eventsOne.data()
                                   ? eventQueue.eventsTwo
                                   : eventQueue.eventsOne;
    eventQueue.eventsIndex = 0;
}

template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<TraceEventWithArguments> &eventQueue);
template NANOTRACE_EXPORT void flushInThread(EnabledEventQueue<TraceEventWithoutArguments> &eventQueue);

template<typename TraceEvent>
EventQueue<TraceEvent, Tracing::IsEnabled>::EventQueue(std::shared_ptr<EnabledTraceFile> file)
    : file{std::move(file)}
    , threadId{std::this_thread::get_id()}
{
    setEventsSpans(*eventArrayOne.get(), *eventArrayTwo.get());
    Internal::EventQueueTracker<TraceEvent>::get().addQueue(this);
    if (QThread::currentThread()) {
        auto name = getThreadName();
        if (name.size()) {
            writeMetaEvent(*this->file, "thread_name", name);
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
    std::lock_guard lock{mutex};
    if (isEnabled == IsEnabled::Yes && eventsIndex > 0) {
        flushEvents(currentEvents.subspan(0, eventsIndex), threadId, file);
        eventsIndex = 0;
    }
    file.reset();
}

template class NANOTRACE_EXPORT_TEMPLATE EventQueue<TraceEventWithArguments, Tracing::IsEnabled>;
template class NANOTRACE_EXPORT_TEMPLATE EventQueue<TraceEventWithoutArguments, Tracing::IsEnabled>;

} // namespace NanotraceHR
