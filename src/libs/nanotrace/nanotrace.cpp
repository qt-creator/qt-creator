/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/


#include "nanotrace.h"

#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Nanotrace
{

Arg::Arg(const std::string &name, const SupportedType &val)
    : m_name(name)
    , m_value(val)
{ }

std::string Arg::name() const
{
    return m_name;
}

struct ConvertArgValueToString {
    std::string operator()(const int &v)         { return std::to_string(v); }
    std::string operator()(const int64_t &v)     { return std::to_string(v); }
    std::string operator()(const double &v)      { return std::to_string(v); }
    std::string operator()(const std::string &v) { return "\"" + v + "\""; }
};

std::string Arg::value() const
{
    return std::visit(ConvertArgValueToString(), m_value);
}


struct InitEvent
{
    bool initialized = false;

    int32_t pid;
    std::string processName;

    std::thread::id tid;
    std::string threadName;

    std::string filePath;

    TimePoint ts;

    int64_t overhead;
};


struct TraceEvent
{
    int32_t pid;

    std::thread::id tid;

    std::string name;
    std::string cat;

    char ph;

    int64_t oh;
    int64_t ts;

    std::vector< Nanotrace::Arg > args;

    friend std::ostream& operator<<(std::ostream& stream, const TraceEvent& event);
};


constexpr size_t eventCount = 10000;

static std::vector< TraceEvent > events;

static thread_local InitEvent initEvent;

static int32_t getProcessId() {
#ifdef _WIN32
return static_cast< int32_t >(GetCurrentProcessId());
#else
return static_cast< int32_t >(getpid());
#endif
}

std::ostream& operator<<(std::ostream &stream, const TraceEvent &event)
{
    stream
        << "{ \"cat\":\"" << event.cat
        << "\", \"pid\":" << event.pid
        << ", \"tid\":\"" << event.tid << "\""
        << ", \"ts\":" << event.ts
        << ", \"ph\":\"" << event.ph
        << "\", \"name\":\"" << event.name
        << "\", \"args\": { \"overhead\": " << event.oh;

    for (auto&& arg : event.args)
        stream << ", \"" << arg.name() <<"\": " << arg.value();

    stream << " } }";
    return stream;
}


void init(const std::string &process, const std::string &thread, const std::string &path)
{
    auto now = Clock::now();
    initEvent.initialized = true;
    initEvent.pid = getProcessId();
    initEvent.processName = process;
    initEvent.tid = std::this_thread::get_id();
    initEvent.threadName = thread;
    initEvent.filePath = path;
    initEvent.ts = now;

    events.reserve(eventCount);
    events.push_back(TraceEvent {
        getProcessId(),
        std::this_thread::get_id(),
        "process_name",
        "",
        'M',
        0,
        0,
        {{"name", process}}
    } );

    events.push_back(TraceEvent {
        getProcessId(),
        std::this_thread::get_id(),
        "thread_name",
        "",
        'M',
        0,
        0,
        {{"name", thread}}
    } );

    if (std::ofstream stream(path, std::ios::trunc); stream.good())
        stream << "{ \"traceEvents\": [\n";
    else
        std::cout << "Nanotrace::init: stream not good" << std::endl;

    events.push_back(TraceEvent {
        getProcessId(),
        std::this_thread::get_id(),
        "Initialize",
        "Initialize",
        'I',
        0,
        std::chrono::duration_cast< Units >(Clock::now() - now).count(),
        {}
    } );

    initEvent.overhead = std::chrono::duration_cast< Units >(Clock::now() - now).count();
}

void shutdown()
{
    flush();

    if (std::ofstream stream(initEvent.filePath, std::ios::app); stream.good())
        stream << "\n] }";
    else
        std::cout << "Nanotrace::shutdown: stream not good" << std::endl;
}

void addTracePoint(
    const std::string &name,
    const std::string &cat,
    char phase,
    std::initializer_list< Nanotrace::Arg > arguments)
{
    if (!initEvent.initialized)
        return;

    auto now = Clock::now();
    auto beg = std::chrono::duration_cast< Units >(now - initEvent.ts);

    events.push_back( TraceEvent {
        getProcessId(),
        std::this_thread::get_id(),
        name,
        cat,
        phase,
        initEvent.overhead,
        beg.count(),
        {arguments}
    } );

    if (events.size() >= eventCount - 1)
        flush();

    initEvent.overhead = std::chrono::duration_cast< Units >(Clock::now() - now).count();
}

void flush()
{
    if (events.empty())
        return;

    if (std::ofstream stream(initEvent.filePath, std::ios::app); stream.good()) {
        stream << events[0];
        for (size_t i=1; i<events.size(); ++i) {
            stream << ",\n" << events[i];
        }
    } else {
        std::cout << "Nanotrace::flush: stream not good" << std::endl;
    }

    events.clear();
}

ScopeTracer::ScopeTracer(
    const std::string &name,
    const std::string &cat,
    std::initializer_list< Nanotrace::Arg > arguments)
    : m_start(Clock::now())
    , m_name(name)
    , m_cat(cat)
    , m_args(arguments)
{ }

ScopeTracer::~ScopeTracer()
{
    if (!initEvent.initialized)
        return;

    auto now = Clock::now();
    auto beg = std::chrono::duration_cast< Units >(m_start - initEvent.ts);

    m_args.push_back(Arg("dur", std::chrono::duration_cast< Units >(now - m_start).count()));

    events.push_back(TraceEvent {
        getProcessId(),
        std::this_thread::get_id(),
        m_name,
        m_cat,
        'X',
        initEvent.overhead,
        beg.count(),
        { m_args }
    } );

    if (events.size() >= eventCount - 1)
        flush();

    initEvent.overhead = std::chrono::duration_cast< Units >(Clock::now() - now).count();
}

} // End namespace Nanotrace.


