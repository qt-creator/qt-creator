/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "invalidmodelnodeexception.h"

/*!
\class QmlDesigner::InvalidModelNodeException
\ingroup CoreExceptions
\brief Exception for a invalid model node

\see ModelNode
*/
namespace QmlDesigner {
/*!
\brief Constructor

\param line use the __LINE__ macro
\param function use the __FUNCTION__ or the Q_FUNC_INFO macro
\param file use the __FILE__ macro
*/
InvalidModelNodeException::InvalidModelNodeException(int line,
                                                     const QString &function,
                                                     const QString &file)
 : Exception(line, function, file)
{
}

/*!
\brief Returns the type of this exception

\returns the type as a string
*/
QString InvalidModelNodeException::type() const
{
    return "InvalidModelNodeException";
}

}
