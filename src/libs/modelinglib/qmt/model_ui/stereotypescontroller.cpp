// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stereotypescontroller.h"

#include <QStringList>

namespace qmt {

StereotypesController::StereotypesController(QObject *parent) :
    QObject(parent)
{
}

bool StereotypesController::isParsable(const QString &stereotypes)
{
    const QStringList list = stereotypes.split(QLatin1Char(','));
    for (const QString &part : list) {
        QString stereotype = part.trimmed();
        if (stereotype.length() == 0)
            return false;
    }
    return true;
}

QString StereotypesController::toString(const QList<QString> &stereotypes)
{
    QString s;
    bool first = true;
    for (const QString &stereotype : stereotypes) {
        if (!first)
            s += ", ";
        s += stereotype;
        first = false;
    }
    return s;
}

QList<QString> StereotypesController::fromString(const QString &stereotypes)
{
    QList<QString> result;
    const QStringList list = stereotypes.split(QLatin1Char(','));
    for (const QString &part : list) {
        QString stereotype = part.trimmed();
        if (stereotype.length() > 0)
            result.append(stereotype);
    }
    return result;
}

} // namespace qmt
