/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "status.h"

#include <QSharedData>
#include <QString>

namespace Valgrind {
namespace XmlProtocol {

class Status::Private : public QSharedData
{
public:
    Private()
        : state(Running)
    {
    }

    State state;
    QString time;
};

Status::Status()
    : d(new Private)
{
}

Status::Status(const Status &other)
    : d(other.d)
{
}

Status::~Status()
{
}

void Status::swap(Status &other)
{
    qSwap(d, other.d);
}

Status &Status::operator=(const Status &other)
{
    Status tmp(other);
    swap(tmp);
    return *this;
}

bool Status::operator==(const Status &other) const
{
    return d->state == other.d->state && d->time == other.d->time;
}

void Status::setState(State state)
{
    d->state = state;
}

Status::State Status::state() const
{
    return d->state;
}

void Status::setTime(const QString &time)
{
    d->time = time;
}

QString Status::time() const
{
    return d->time;
}

} // namespace XmlProtocol
} // namespace Valgrind
