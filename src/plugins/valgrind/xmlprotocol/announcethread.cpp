// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "announcethread.h"
#include "frame.h"

#include <QSharedData>
#include <QList>

#include <algorithm>

namespace Valgrind::XmlProtocol {

class AnnounceThread::Private : public QSharedData
{
public:
    qint64 hThreadId = -1;
    QList<Frame> stack;
};

AnnounceThread::AnnounceThread()
    : d(new Private)
{
}

AnnounceThread::AnnounceThread(const AnnounceThread &other) = default;

AnnounceThread::~AnnounceThread() = default;

void AnnounceThread::swap(AnnounceThread &other)
{
    qSwap(d, other.d);
}

AnnounceThread &AnnounceThread::operator=(const AnnounceThread &other)
{
    AnnounceThread tmp(other);
    swap(tmp);
    return *this;
}

bool AnnounceThread::operator==(const AnnounceThread &other) const
{
    return d->stack == other.d->stack
            && d->hThreadId == other.d->hThreadId;
}

qint64 AnnounceThread::helgrindThreadId() const
{
    return d->hThreadId;
}

void AnnounceThread::setHelgrindThreadId(qint64 id)
{
    d->hThreadId = id;
}

QList<Frame> AnnounceThread::stack() const
{
    return d->stack;
}

void AnnounceThread::setStack(const QList<Frame> &stack)
{
    d->stack = stack;
}

} // namespace Valgrind::XmlProtocol
