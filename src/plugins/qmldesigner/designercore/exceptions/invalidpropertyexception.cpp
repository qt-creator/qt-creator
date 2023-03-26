// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidpropertyexception.h"
/*!
\class QmlDesigner::InvalidPropertyException
\ingroup CoreExceptions
\brief The InvalidPropertyException class provides an exception for an invalid
property.

\see AbstractProperty
*/
namespace QmlDesigner {
/*!
    Constructs an exception. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
InvalidPropertyException::InvalidPropertyException(int line,
                                                   const QByteArray &function,
                                                   const QByteArray &file,
                                                   const QByteArray &argument)
 : Exception(line, function, file), m_argument(QString::fromLatin1(argument))
{
    createWarning();
}

/*!
    Returns the type of this exception as a string.
*/
QString InvalidPropertyException::type() const
{
    return QLatin1String("InvalidPropertyException");
}

/*!
    Returns the argument of the property of this exception as a string.
*/
QString InvalidPropertyException::argument() const
{
    return m_argument;
}

} // namespace QmlDesigner
