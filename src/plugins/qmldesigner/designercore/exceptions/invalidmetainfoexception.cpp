// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidmetainfoexception.h"

/*!
\class QmlDesigner::InvalidMetaInfoException
\ingroup CoreExceptions
\brief The InvalidMetaInfoException class provides an exception for invalid meta
info.

\see NodeMetaInfo PropertyMetaInfo MetaInfo
*/
namespace QmlDesigner {
/*!
    Constructs an exception. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
InvalidMetaInfoException::InvalidMetaInfoException(int line,
                                                           const QByteArray &function,
                                                           const QByteArray &file)
 : Exception(line, function, file)
{
    createWarning();
}

/*!
    Returns the type of this exception as a string.
*/
QString InvalidMetaInfoException::type() const
{
    return QLatin1String("InvalidMetaInfoException");
}

}
