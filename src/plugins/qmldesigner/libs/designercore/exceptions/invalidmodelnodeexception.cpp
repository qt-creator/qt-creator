// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidmodelnodeexception.h"

/*!
\class QmlDesigner::InvalidModelNodeException
\ingroup CoreExceptions
\brief The InvalidModelNodeException class provides an exception for an invalid
model node.

\see ModelNode
*/
namespace QmlDesigner {
/*!
    Constructs an exception. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
InvalidModelNodeException::InvalidModelNodeException(int line,
                                                     const QByteArray &function,
                                                     const QByteArray &file)
 : Exception(line, function, file)
{
    createWarning();
}

/*!
    Returns the type of this exception as a string.
*/
QString InvalidModelNodeException::type() const
{
    return QLatin1String("InvalidModelNodeException");
}

}
