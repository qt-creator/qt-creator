/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "invalidreparentingexception.h"
/*!
\class QmlDesigner::InvalidReparentingException
\ingroup CoreExceptions
    \brief The InvalidReparentingException class provides an exception for
    invalid reparenting.

\see ModelNode
*/
namespace QmlDesigner {
/*!
    Constructs an exception. \a line uses the __LINE__ macro,
    \a function uses the __FUNCTION__ or the Q_FUNC_INFO macro, and \a file uses
    the __FILE__ macro.
*/
InvalidReparentingException::InvalidReparentingException(int line,
                                                         const QByteArray &function,
                                                         const QByteArray &file)
 : Exception(line, function, file)
{
    createWarning();
}

/*!
Returns the type of this exception as a string.
*/
QString InvalidReparentingException::type() const
{
    return QLatin1String("InvalidReparentingException");
}
} // namespace QmlDesigner
