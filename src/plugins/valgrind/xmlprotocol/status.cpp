// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "status.h"

#include <QSharedData>
#include <QString>

namespace Valgrind::XmlProtocol {

class Status::Private : public QSharedData
{
public:
    State state = Running;
    QString time;
};

Status::Status()
    : d(new Private)
{
}

Status::Status(const Status &other) = default;

Status::~Status() = default;

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

} // namespace Valgrind::XmlProtocol
