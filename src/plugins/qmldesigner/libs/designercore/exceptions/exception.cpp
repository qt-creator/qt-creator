// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "exception.h"

#ifdef Q_OS_LINUX
#include <execinfo.h>
#include <cxxabi.h>
#endif

#include <utils/qtcassert.h>

#include <QCoreApplication>

#include <functional>


namespace QmlDesigner {

bool Exception::s_warnAboutException = true;

void Exception::setWarnAboutException(bool warn)
{
    s_warnAboutException = warn;
}

bool Exception::warnAboutException()
{
    return s_warnAboutException;
}

namespace {
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
QString getBackTrace()
{
    auto trace = std::stacktrace::current();
    return QString::fromStdString(std::to_string(trace));
}
#endif
} // namespace

QString Exception::defaultDescription(const Sqlite::source_location &sourceLocation)
{
    return QStringView(u"file: %1, function: %2, line: %3")
        .arg(QLatin1StringView{sourceLocation.file_name()},
             QLatin1StringView{sourceLocation.function_name()},
             QString::number(sourceLocation.line()));
}

void Exception::createWarning() const
{
    if (warnAboutException())
        qDebug() << *this;
}

namespace {
std::function<void(QStringView title, QStringView description)> showExceptionCallback;
}

/*!
    Shows message in a message box.
*/
void Exception::showException([[maybe_unused]] const QString &title) const
{
    if (showExceptionCallback)
        showExceptionCallback(title, description());
}

void Exception::setShowExceptionCallback(std::function<void(QStringView, QStringView)> callback)
{
    showExceptionCallback = std::move(callback);
}

/*!
    Returns the line number where this exception was thrown as an integer.
*/
int Exception::line() const
{
    return location().line();
}

/*!
    Returns the file name where this exception was thrown as a string.
*/
QString Exception::file() const
{
    return QString::fromUtf8(location().file_name());
}

QDebug operator<<(QDebug debug, const Exception &exception)
{
    const auto &location = exception.location();
    debug.nospace() << "Exception: " << exception.type()
                    << "\n"
                       "Function:  "
                    << location.function_name()
                    << "\n"
                       "File:      "
                    << location.file_name()
                    << "\n"
                       "Line:      "
                    << location.line() << "\n";
    if (!exception.description().isEmpty())
        debug.nospace() << exception.description() << "\n";

    return debug.space();
}
}
