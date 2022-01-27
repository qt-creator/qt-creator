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

#pragma once

#include <QtGlobal>

#if defined(NANOTRACE_LIBRARY)
#  define NANOTRACESHARED_EXPORT Q_DECL_EXPORT
#else
#  define NANOTRACESHARED_EXPORT Q_DECL_IMPORT
#endif


#include <string>
#include <variant>
#include <vector>


#ifdef NANOTRACE_ENABLED

#define NANOTRACE_INIT(process, thread, filepath) Nanotrace::init(process, thread, filepath)
#define NANOTRACE_SHUTDOWN() Nanotrace::shutdown()

#define NANOTRACE_SCOPE(cat, name) Nanotrace::ScopeTracer nanotraceScopedTracerObject(name, cat, {})
#define NANOTRACE_INSTANT(cat, name) Nanotrace::addTracePoint(name, cat, 'I', {})
#define NANOTRACE_BEGIN(cat, name) Nanotrace::addTracePoint(name, cat, 'B', {})
#define NANOTRACE_END(cat, name) Nanotrace::addTracePoint(name, cat, 'E', {})

#define NANOTRACE_SCOPE_ARGS(cat, name, ...) Nanotrace::ScopeTracer nanotraceScopedTracerObject(name, cat, {__VA_ARGS__})
#define NANOTRACE_INSTANT_ARGS(cat, name, ...) Nanotrace::addTracePoint(name, cat, 'I', {__VA_ARGS__})
#define NANOTRACE_BEGIN_ARGS(cat, name, ...) Nanotrace::addTracePoint(name, cat, 'B', {__VA_ARGS__})

#else

#define NANOTRACE_INIT(process, thread, filepath)
#define NANOTRACE_SHUTDOWN()

#define NANOTRACE_SCOPE(cat, name)
#define NANOTRACE_INSTANT(cat, name)
#define NANOTRACE_BEGIN(cat, name)
#define NANOTRACE_END(cat, name)

#define NANOTRACE_SCOPE_ARGS(cat, name, ...)
#define NANOTRACE_INSTANT_ARGS(cat, name, ...)
#define NANOTRACE_BEGIN_ARGS(cat, name, ...)

#endif


namespace Nanotrace
{

using Units = std::chrono::nanoseconds;
using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point< Clock >;

class NANOTRACESHARED_EXPORT Arg
{
public:
    using SupportedType = std::variant<int, int64_t, double, std::string>;

    Arg(const std::string &name, const SupportedType &val);
    std::string name() const;
    std::string value() const;

private:
    std::string m_name;
    SupportedType m_value;
};

NANOTRACESHARED_EXPORT void init(const std::string &process, const std::string &thread, const std::string &path);

NANOTRACESHARED_EXPORT void shutdown();

NANOTRACESHARED_EXPORT void flush();

NANOTRACESHARED_EXPORT void addTracePoint(
    const std::string &name,
    const std::string &cat,
    char phase,
    std::initializer_list< Nanotrace::Arg > arguments);

class NANOTRACESHARED_EXPORT ScopeTracer
{
public:
    ScopeTracer(
        const std::string &name,
        const std::string &cat,
        std::initializer_list< Nanotrace::Arg > arguments);
    ~ScopeTracer();

private:
    TimePoint m_start;
    std::string m_name;
    std::string m_cat;
    std::vector< Nanotrace::Arg > m_args;
};

} // End namespace Nanotrace.

