/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "stack.h"
#include "frame.h"

#include <QtCore/QSharedData>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <algorithm>

using namespace Valgrind;
using namespace Valgrind::XmlProtocol;

class Stack::Private : public QSharedData
{
public:
    Private()
        : line(-1)
        , hthreadid(-1)
    {
    }

    QString auxwhat;
    QString file;
    QString dir;
    qint64 line;
    qint64 hthreadid;
    QVector<Frame> frames;
};

Stack::Stack()
    : d(new Private)
{
}

Stack::Stack(const Stack &other)
    : d(other.d)
{
}

Stack::~Stack()
{
}

void Stack::swap(Stack &other)
{
    qSwap(d, other.d);
}

Stack &Stack::operator=(const Stack &other)
{
    Stack tmp(other);
    swap(tmp);
    return *this;
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

QVector<Frame> Stack::frames() const
{
    return d->frames;
}

void Stack::setFrames(const QVector<Frame> &frames)
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
