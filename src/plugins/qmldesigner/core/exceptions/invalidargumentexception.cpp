/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "invalidargumentexception.h"

/*!
\class QmlDesigner::InvalidArgumentException
\ingroup CoreExceptions
\brief Exception for a invalid argument

*/
namespace QmlDesigner {

/*!
\brief Constructor

\param line use the __LINE__ macro
\param function use the __FUNCTION__ or the Q_FUNC_INFO macro
\param file use the __FILE__ macro
*/
InvalidArgumentException::InvalidArgumentException(int line,
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
QString InvalidArgumentException::type() const
{
    return "InvalidArgumentException";
}

/*!
\brief Returns the argument of this exception

\returns the argument as a string
*/
QString InvalidArgumentException::argument() const
{
    return m_argument;
}

}
