// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidargumentexception.h"
#include <QString>
#include <QCoreApplication>
/*!
\class QmlDesigner::InvalidArgumentException
\ingroup CoreExceptions
\brief The InvalidArgumentException class provides an exception for an invalid
argument.

*/
namespace QmlDesigner {

QString InvalidArgumentException::invalidArgumentDescription(int line,
                                                             const QByteArray &function,
                                                             const QByteArray &file,
                                                             const QByteArray &argument)
{
    if (QString::fromUtf8(function) == QLatin1String("createNode")) {
        return QCoreApplication::translate("QmlDesigner::InvalidArgumentException",
                  "Failed to create item of type %1").arg(QString::fromUtf8(argument));
    }

    return Exception::defaultDescription(line, function, file);
}

/*!
    Constructs the exception for \a argument. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
InvalidArgumentException::InvalidArgumentException(int line,
                                                   const QByteArray &function,
                                                   const QByteArray &file,
                                                   const QByteArray &argument)
    : InvalidArgumentException(line, function, file, argument,
                               invalidArgumentDescription(line, function, file, argument))
{
    createWarning();
}

InvalidArgumentException::InvalidArgumentException(int line,
                                                   const QByteArray &function,
                                                   const QByteArray &file,
                                                   const QByteArray &argument,
                                                   const QString &description)
    : Exception(line, function, file, description)
    , m_argument(QString::fromUtf8(argument))
{
    createWarning();
}

/*!
    Returns the type of the exception as a string.
*/
QString InvalidArgumentException::type() const
{
    return QLatin1String("InvalidArgumentException");
}

/*!
    Returns the argument of the exception as a string.
*/
QString InvalidArgumentException::argument() const
{
    return m_argument;
}

}
