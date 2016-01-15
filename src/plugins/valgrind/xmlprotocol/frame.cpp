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

#include "frame.h"

#include <QString>

#include <algorithm>

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
                && fileName == other.fileName
                && directory == other.directory
                && line == other.line;
    }

    quint64 ip;
    QString object;
    QString functionName;
    QString fileName;
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
    std::swap(d, other.d);
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

QString Frame::fileName() const
{
    return d->fileName;
}

void Frame::setFileName(const QString &file)
{
    d->fileName = file;
}

QString Frame::directory() const
{
    return d->directory;
}

void Frame::setDirectory(const QString &directory)
{
    d->directory = directory;
}

QString Frame::filePath() const
{
    QString f;
    if (!directory().isEmpty())
        f.append(directory()).append(QLatin1Char('/'));
    return f.append(fileName());
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
