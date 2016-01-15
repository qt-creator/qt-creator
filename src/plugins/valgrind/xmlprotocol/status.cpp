/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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
