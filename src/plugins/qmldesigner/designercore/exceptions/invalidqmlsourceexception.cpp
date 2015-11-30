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
 :  Exception(line, function, file),
    m_qmlSource(QString::fromUtf8(qmlSource))
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

QString InvalidQmlSourceException::description() const
{
    return m_qmlSource;
}

} // namespace QmlDesigner
