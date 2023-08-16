// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stack.h"
#include "frame.h"

#include <QList>
#include <QSharedData>
#include <QString>

namespace Valgrind::XmlProtocol {

class Stack::Private : public QSharedData
{
public:
    QString auxwhat;
    QString file;
    QString dir;
    qint64 line = -1;
    qint64 hthreadid = -1;
    QList<Frame> frames;
};

Stack::Stack()
    : d(new Private)
{
}

Stack::Stack(const Stack &other) = default;

Stack::~Stack() = default;

void Stack::swap(Stack &other)
{
    qSwap(d, other.d);
}

void Stack::operator=(const Stack &other)
{
    Stack tmp(other);
    swap(tmp);
}

bool Stack::operator==(const Stack &other) const
{
    return d->frames == other.d->frames
            && d->auxwhat == other.d->auxwhat
            && d->file == other.d->file
            && d->dir == other.d->dir
            && d->line == other.d->line
            && d->hthreadid == other.d->hthreadid;
}

QString Stack::auxWhat() const
{
    return d->auxwhat;
}

void Stack::setAuxWhat(const QString &auxwhat)
{
    d->auxwhat = auxwhat;
}

QList<Frame> Stack::frames() const
{
    return d->frames;
}

void Stack::setFrames(const QList<Frame> &frames)
{
    d->frames = frames;
}

QString Stack::file() const
{
    return d->file;
}

void Stack::setFile(const QString &file)
{
    d->file = file;
}

QString Stack::directory() const
{
    return d->dir;
}

void Stack::setDirectory(const QString &directory)
{
    d->dir = directory;
}

qint64 Stack::line() const
{
    return d->line;
}

void Stack::setLine(qint64 line)
{
    d->line = line;
}

qint64 Stack::helgrindThreadId() const
{
    return d->hthreadid;
}

void Stack::setHelgrindThreadId(qint64 id)
{
    d->hthreadid = id;
}

} // namespace Valgrind::XmlProtocol
