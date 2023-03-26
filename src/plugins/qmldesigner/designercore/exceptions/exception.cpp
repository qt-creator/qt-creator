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

/*!
\defgroup CoreExceptions
*/
/*!
\class QmlDesigner::Exception
\ingroup CoreExceptions
\brief The Exception class is the abstract base class for all exceptions.

    Exceptions should be used if there is no other way to indicate that
    something is going wrong. For example,
    the result would be a inconsistent model or a crash.
*/


namespace QmlDesigner {

#ifdef Q_OS_LINUX
const char* demangle(const char* name)
{
   char buf[1024];
   size_t size = 1024;
   int status;
   char* res;
   res = abi::__cxa_demangle(name,
     buf,
     &size,
     &status);
   return res;
}
#else
const char* demangle(const char* name)
{
   return name;
}
#endif


bool Exception::s_shouldAssert = false;

void Exception::setShouldAssert(bool assert)
{
    s_shouldAssert = assert;
}

bool Exception::shouldAssert()
{
    return s_shouldAssert;
}

bool Exception::s_warnAboutException = true;

void Exception::setWarnAboutException(bool warn)
{
    s_warnAboutException = warn;
}

bool Exception::warnAboutException()
{
    return s_warnAboutException;
}

#ifdef Q_OS_LINUX
static QString getBackTrace()
{
    QString backTrace;
    void * array[50];
    int nSize = backtrace(array, 50);
    char ** symbols = backtrace_symbols(array, nSize);

    for (int i = 0; i < nSize; i++)
        backTrace.append(QString("%1\n").arg(QLatin1String(symbols[i])));

    free(symbols);

    return backTrace;
}
#endif

QString Exception::defaultDescription(int line, const QByteArray &function, const QByteArray &file)
{
    return QString(QStringLiteral("file: %1, function: %2, line: %3"))
            .arg(QString::fromUtf8(file), QString::fromUtf8(function), QString::number(line));
}

/*!
    Constructs an exception. \a line uses the __LINE__ macro, \a function uses
    the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
Exception::Exception(int line, const QByteArray &function, const QByteArray &file)
  : Exception(line, function, file, Exception::defaultDescription(line, function, file))
{ }

Exception::Exception(int line, const QByteArray &function,
                     const QByteArray &file, const QString &description)
  : m_line(line)
  , m_function(QString::fromUtf8(function))
  , m_file(QString::fromUtf8(file))
  , m_description(description)
  #ifdef Q_OS_LINUX
  , m_backTrace(getBackTrace())
  #endif
{
    if (s_shouldAssert) {
        qDebug() << Exception::description();
        QTC_ASSERT(false, ;);
        Q_ASSERT(false);
    }
}

Exception::~Exception() = default;

/*!
    Returns the unmangled backtrace of this exception as a string.
*/
QString Exception::backTrace() const
{
    return m_backTrace;
}

void Exception::createWarning() const
{
    if (warnAboutException())
        qDebug() << *this;
}

/*!
    Returns the optional description of this exception as a string.
*/
QString Exception::description() const
{
    return m_description;
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
        showExceptionCallback(title, m_description);
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
    return m_line;
}

/*!
    Returns the function name where this exception was thrown as a string.
*/
QString Exception::function() const
{
    return m_function;
}

/*!
    Returns the file name where this exception was thrown as a string.
*/
QString Exception::file() const
{
    return m_file;
}

QDebug operator<<(QDebug debug, const Exception &exception)
{
    debug.nospace() << "Exception: " << exception.type() << "\n"
                       "Function:  " << exception.function() << "\n"
                       "File:      " << exception.file() << "\n"
                       "Line:      " << exception.line() << "\n";
    if (!exception.description().isEmpty())
        debug.nospace() << exception.description() << "\n";

    if (!exception.backTrace().isEmpty())
        debug.nospace().noquote() << exception.backTrace();

    return debug.space();
}

/*!
\fn QString Exception::type() const
Returns the type of this exception as a string.
*/
}
