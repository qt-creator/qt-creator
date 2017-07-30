/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "stereotypescontroller.h"

#include <QStringList>

namespace qmt {

StereotypesController::StereotypesController(QObject *parent) :
    QObject(parent)
{
}

bool StereotypesController::isParsable(const QString &stereotypes)
{
    QStringList list = stereotypes.split(QLatin1Char(','));
    foreach (const QString &part, list) {
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
    foreach (const QString &stereotype, stereotypes) {
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
    QStringList list = stereotypes.split(QLatin1Char(','));
    foreach (const QString &part, list) {
        QString stereotype = part.trimmed();
        if (stereotype.length() > 0)
            result.append(stereotype);
    }
    return result;
}

} // namespace qmt
