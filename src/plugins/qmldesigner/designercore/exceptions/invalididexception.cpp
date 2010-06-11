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

#include <QCoreApplication>
#include "invalididexception.h"

namespace QmlDesigner {

InvalidIdException::InvalidIdException(int line,
                                       const QString &function,
                                       const QString &file,
                                       const QString &id,
                                       Reason reason) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(id)
{
    if (reason == InvalidCharacters) {
        m_description = QCoreApplication::translate("InvalidIdException", "Only alphanumeric characters and underscore allowed.\nIds must begin with a lowercase letter.");
    } else {
        m_description = QCoreApplication::translate("InvalidIdException", "Ids have to be unique.");
    }
}

InvalidIdException::InvalidIdException(int line,
                                       const QString &function,
                                       const QString &file,
                                       const QString &id,
                                       const QString &description) :
    InvalidArgumentException(line, function, file, "id"),
    m_id(id),
    m_description(description)
{
}

QString InvalidIdException::type() const
{
    return "InvalidIdException";
}

QString InvalidIdException::description() const
{
    return QCoreApplication::translate("InvalidIdException", "Invalid Id: %1\n%2").arg(m_id, m_description);
}

}
