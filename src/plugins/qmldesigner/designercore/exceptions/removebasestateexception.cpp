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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "removebasestateexception.h"

/*!
\class QmlDesigner::RemoveBaseStateException
\ingroup CoreExceptions
    \brief The RemoveBaseStateException class provides an exception if you try
    to remove a BaseState.

/see NodeState ModelState
*/
namespace QmlDesigner {
/*!
    Constructs an exception. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
RemoveBaseStateException::RemoveBaseStateException(int line,
                                             const QByteArray &function,
                                             const QByteArray &file)
  : Exception(line, function, file)
{
    createWarning();
}

/*!
Returns the type of this exception as a string.
*/
QString RemoveBaseStateException::type() const
{
    return QLatin1String("RemoveBaseStateException");
}

} // namespace QmlDesigner
