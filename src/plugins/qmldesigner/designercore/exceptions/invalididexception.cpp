// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
