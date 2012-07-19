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
