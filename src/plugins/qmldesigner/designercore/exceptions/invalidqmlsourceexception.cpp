// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidqmlsourceexception.h"

/*!
\class QmlDesigner::InvalidQmlSourceException
\ingroup CoreExceptions
\brief The InvalidQmlSourceException class provides an exception for invalid QML
source code.

*/
namespace QmlDesigner {
/*!
    Constructs an exception for \qmlSource. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
InvalidQmlSourceException::InvalidQmlSourceException(int line,
                              const QByteArray &function,
                              const QByteArray &file,
                              const QByteArray &qmlSource)
 :  Exception(line, function, file, QString::fromUtf8(qmlSource))
{
    createWarning();
}

/*!
    Returns the type of the exception as a string.
*/
QString InvalidQmlSourceException::type() const
{
    return QLatin1String("InvalidQmlSourceException");
}

} // namespace QmlDesigner
