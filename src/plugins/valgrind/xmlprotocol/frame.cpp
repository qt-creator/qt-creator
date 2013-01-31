/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "frame.h"

#include <QString>
#include <QtAlgorithms>

namespace Valgrind {
namespace XmlProtocol {

class Frame::Private : public QSharedData
{
public:
    Private() : ip(0), line(-1) {}

    bool operator==(const Private &other) const
    {
        return ip == other.ip
                && object == other.object
                && functionName == other.functionName
                && file == other.file
                && directory == other.directory
                && line == other.line;
    }

    quint64 ip;
    QString object;
    QString functionName;
    QString file;
    QString directory;
    int line;
};

Frame::Frame() : d(new Private)
{
}

Frame::~Frame()
{
}

Frame::Frame(const Frame &other) :
    d(other.d)
{
}

Frame &Frame::operator=(const Frame &other)
{
    Frame tmp(other);
    swap(tmp);
    return *this;
}

bool Frame::operator==( const Frame &other ) const
{
    return *d == *other.d;
}

bool Frame::operator!=(const Frame &other) const
{
    return !(*this == other);
}

void Frame::swap(Frame &other)
{
    qSwap(d, other.d);
}

quint64 Frame::instructionPointer() const
{
    return d->ip;
}

void Frame::setInstructionPointer(quint64 ip)
{
    d->ip = ip;
}

QString Frame::object() const
{
    return d->object;
}

void Frame::setObject(const QString &obj)
{
    d->object = obj;
}

QString Frame::functionName() const
{
    return d->functionName;
}

void Frame::setFunctionName(const QString &functionName)
{
    d->functionName = functionName;
}

QString Frame::file() const
{
    return d->file;
}

void Frame::setFile(const QString &file)
{
    d->file = file;
}

QString Frame::directory() const
{
    return d->directory;
}

void Frame::setDirectory(const QString &directory)
{
    d->directory = directory;
}

int Frame::line() const
{
    return d->line;
}

void Frame::setLine(int line)
{
    d->line = line;
}

} // namespace XmlProtocol
} // namespace Valgrind
