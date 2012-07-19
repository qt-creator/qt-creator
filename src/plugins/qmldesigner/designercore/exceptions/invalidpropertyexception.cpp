/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "invalidpropertyexception.h"
/*!
\class QmlDesigner::InvalidPropertyException
\ingroup CoreExceptions
\brief Exception for a invalid property

\see AbstractProperty
*/
namespace QmlDesigner {
/*!
\brief Constructor

\param line use the __LINE__ macro
\param function use the __FUNCTION__ or the Q_FUNC_INFO macro
\param file use the __FILE__ macro
*/
InvalidPropertyException::InvalidPropertyException(int line,
                                                   const QString &function,
                                                   const QString &file,
                                                   const QString &argument)
 : Exception(line, function, file), m_argument(argument)
{
}

/*!
\brief Returns the type of this exception

\returns the type as a string
*/
QString InvalidPropertyException::type() const
{
    return "InvalidPropertyException";
}

/*!
\brief Returns the argument of the property of this exception

\returns the argument as a string
*/
QString InvalidPropertyException::argument() const
{
    return m_argument;
}

} // namespace QmlDesigner
