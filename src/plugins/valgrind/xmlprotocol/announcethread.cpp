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

#include "announcethread.h"
#include "frame.h"

#include <QSharedData>
#include <QVector>

#include <algorithm>

namespace Valgrind {
namespace XmlProtocol {

class AnnounceThread::Private : public QSharedData
{
public:
    Private()
        : hThreadId( -1 )
    {
    }

    qint64 hThreadId;
    QVector<Frame> stack;
};

AnnounceThread::AnnounceThread()
    : d(new Private)
{
}

AnnounceThread::AnnounceThread(const AnnounceThread &other)
    : d(other.d)
{
}

AnnounceThread::~AnnounceThread()
{
}

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

QVector<Frame> AnnounceThread::stack() const
{
    return d->stack;
}

void AnnounceThread::setStack(const QVector<Frame> &stack)
{
    d->stack = stack;
}

} // namespace XmlProtocol
} // namespace Valgrind
