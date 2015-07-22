/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "exception.h"

#ifdef Q_OS_LINUX
#include <execinfo.h>
#include <cxxabi.h>
#endif

#include <QCoreApplication>

#include <coreplugin/messagebox.h>

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

bool Exception::warnAboutException()
{
    static bool warnException = !qgetenv("QTCREATOR_QTQUICKDESIGNER_WARN_EXCEPTION").isEmpty();
    return warnException;
}

/*!
    Constructs an exception. \a line uses the __LINE__ macro, \a function uses
    the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
Exception::Exception(int line,
              const QByteArray &_function,
              const QByteArray &_file)
  : m_line(line),
    m_function(QString::fromUtf8(_function)),
    m_file(QString::fromUtf8(_file))
{
#ifdef Q_OS_LINUX
    void * array[50];
    int nSize = backtrace(array, 50);
    char ** symbols = backtrace_symbols(array, nSize);

    for (int i = 0; i < nSize; i++)
    {
        m_backTrace.append(QString("%1\n").arg(symbols[i]));
    }

    free(symbols);
#endif

if (s_shouldAssert)
    Q_ASSERT_X(false, _function, QString(QStringLiteral("%1:%2 - %3")).arg(m_file).arg(m_line).arg(m_function).toUtf8());
}

Exception::~Exception()
{
}

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
    return QString(QStringLiteral("file: %1, function: %2, line: %3")).arg(m_file, m_function, QString::number(m_line));
}

/*!
    Shows message in a message box.
*/
void Exception::showException(const QString &title) const
{
    QString composedTitle = title.isEmpty() ? QCoreApplication::translate("QmlDesigner", "Error") : title;
    Core::AsynchronousMessageBox::warning(composedTitle, description());
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
        debug.nospace() << exception.description();

    if (!exception.backTrace().isEmpty())
        debug.nospace() << exception.backTrace();

    return debug.space();
}

/*!
\fn QString Exception::type() const
Returns the type of this exception as a string.
*/
}
