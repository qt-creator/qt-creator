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

#include "invalididexception.h"

#include <QCoreApplication>

namespace QmlDesigner {

InvalidIdException::InvalidIdException(int line,
                                       const QByteArray &function,
                                       const QByteArray &file,
                                       const QByteArray &id,
                                       Reason reason) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(QString::fromUtf8(id))
{
    if (reason == InvalidCharacters)
        m_description = QCoreApplication::translate("InvalidIdException", "Only alphanumeric characters and underscore allowed.\nIds must begin with a lowercase letter.");
    else
        m_description = QCoreApplication::translate("InvalidIdException", "Ids have to be unique.");
}

InvalidIdException::InvalidIdException(int line,
                                       const QByteArray &function,
                                       const QByteArray &file,
                                       const QByteArray &id,
                                       const QByteArray &description) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(QString::fromUtf8(id)),
    m_description(QString::fromUtf8(description))
{
    createWarning();
}

QString InvalidIdException::type() const
{
    return QLatin1String("InvalidIdException");
}

QString InvalidIdException::description() const
{
    return QCoreApplication::translate("InvalidIdException", "Invalid Id: %1\n%2").arg(m_id, m_description);
}

}
