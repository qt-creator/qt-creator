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

#include "invalididexception.h"

#include <QCoreApplication>

namespace QmlDesigner {

static QString descriptionBasedOnReason(InvalidIdException::Reason reason)
{
    if (reason == InvalidIdException::InvalidCharacters)
        return QCoreApplication::translate("InvalidIdException",
                                           "Only alphanumeric characters and underscore allowed.\n"
                                           "Ids must begin with a lowercase letter.");

    return QCoreApplication::translate("InvalidIdException", "Ids have to be unique.");
}

static QString decorateDescriptionWithId(const QString &id, const QString &description)
{
    return QCoreApplication::translate("InvalidIdException", "Invalid Id: %1\n%2")
            .arg(id, description);
}

InvalidIdException::InvalidIdException(int line,
                                       const QByteArray &function,
                                       const QByteArray &file,
                                       const QByteArray &id,
                                       Reason reason)
    : InvalidArgumentException(line, function, file, "id",
                               decorateDescriptionWithId(QString::fromUtf8(id),
                                                         descriptionBasedOnReason(reason)))
{ }

InvalidIdException::InvalidIdException(int line,
                                       const QByteArray &function,
                                       const QByteArray &file,
                                       const QByteArray &id,
                                       const QByteArray &description)
    : InvalidArgumentException(line, function, file, "id",
                               decorateDescriptionWithId(QString::fromUtf8(id),
                                                         QString::fromUtf8(description)))
{
    createWarning();
}

QString InvalidIdException::type() const
{
    return QLatin1String("InvalidIdException");
}

}
