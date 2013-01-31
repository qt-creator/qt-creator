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

#include "error.h"
#include "frame.h"
#include "stack.h"
#include "suppression.h"

#include <QSharedData>
#include <QString>
#include <QTextStream>
#include <QVector>

#include <QtAlgorithms>

namespace Valgrind {
namespace XmlProtocol {

class Error::Private : public QSharedData
{
public:
    explicit Private() :
        unique(0),
        tid(0),
        kind(0),
        leakedBytes(0),
        leakedBlocks(0),
        hThreadId(-1)
    {}

    qint64 unique;
    qint64 tid;
    QString what;
    int kind;
    QVector<Stack> stacks;
    Suppression suppression;
    quint64 leakedBytes;
    qint64 leakedBlocks;
    qint64 hThreadId;

    bool operator==(const Private &other) const
    {
        return unique == other.unique
                && tid == other.tid
                && what == other.what
                && kind == other.kind
                && stacks == other.stacks
                && suppression == other.suppression
                && leakedBytes == other.leakedBytes
                && leakedBlocks == other.leakedBlocks
                && hThreadId == other.hThreadId;
    }
};

Error::Error() :
    d(new Private)
{
}

Error::~Error()
{
}

Error::Error(const Error &other) :
    d( other.d )
{
}

void Error::swap(Error &other)
{
    qSwap(d, other.d);
}

Error &Error::operator=(const Error &other)
{
    Error tmp(other);
    swap(tmp);
    return *this;
}

bool Error::operator ==(const Error &other) const
{
    return *d == *other.d;
}

bool Error::operator !=(const Error &other) const
{
    return !(*d == *other.d);
}

Suppression Error::suppression() const
{
    return d->suppression;
}

void Error::setSuppression(const Suppression &supp)
{
    d->suppression = supp;
}

qint64 Error::unique() const
{
    return d->unique;
}

void Error::setUnique(qint64 unique)
{
    d->unique = unique;
}

qint64 Error::tid() const
{
    return d->tid;
}

void Error::setTid(qint64 tid)
{
    d->tid = tid;
}

quint64 Error::leakedBytes() const
{
    return d->leakedBytes;
}

void Error::setLeakedBytes(quint64 l)
{
    d->leakedBytes = l;
}

qint64 Error::leakedBlocks() const
{
    return d->leakedBlocks;
}

void Error::setLeakedBlocks(qint64 b)
{
    d->leakedBlocks = b;
}

QString Error::what() const
{
    return d->what;
}

void Error::setWhat(const QString &what)
{
    d->what = what;
}

int Error::kind() const
{
    return d->kind;
}

void Error::setKind(int k)
{
    d->kind = k;
}

QVector<Stack> Error::stacks() const
{
    return d->stacks;
}

void Error::setStacks(const QVector<Stack> &stacks)
{
    d->stacks = stacks;
}

void Error::setHelgrindThreadId(qint64 id)
{
    d->hThreadId = id;
}

qint64 Error::helgrindThreadId() const
{
    return d->hThreadId;
}

QString Error::toXml() const
{
    QString xml;
    QTextStream stream(&xml);
    stream << "<error>\n";
    stream << "  <unique>" << d->unique << "</unique>\n";
    stream << "  <tid>" << d->tid << "</tid>\n";
    stream << "  <kind>" << d->kind << "</kind>\n";
    if (d->leakedBlocks > 0 && d->leakedBytes > 0) {
        stream << "  <xwhat>\n"
               << "    <text>" << d->what << "</text>\n"
               << "    <leakedbytes>" << d->leakedBytes << "</leakedbytes>\n"
               << "    <leakedblocks>" << d->leakedBlocks << "</leakedblocks>\n"
               << "  </xwhat>\n";
    } else {
        stream << "  <what>" << d->what << "</what>\n";
    }

    foreach (const Stack &stack, d->stacks) {
        if (!stack.auxWhat().isEmpty())
            stream << "  <auxwhat>" << stack.auxWhat() << "</auxwhat>\n";
        stream << "  <stack>\n";

        foreach (const Frame &frame, stack.frames()) {
            stream << "    <frame>\n";
            stream << "      <ip>0x" << QString::number(frame.instructionPointer(), 16) << "</ip>\n";
            if (!frame.object().isEmpty())
                stream << "      <obj>" << frame.object() << "</obj>\n";
            if (!frame.functionName().isEmpty())
                stream << "      <fn>" << frame.functionName() << "</fn>\n";
            if (!frame.directory().isEmpty())
                stream << "      <dir>" << frame.directory() << "</dir>\n";
            if (!frame.file().isEmpty())
                stream << "      <file>" << frame.file() << "</file>\n";
            if (frame.line() != -1)
                stream << "      <line>" << frame.line() << "</line>";
            stream << "    </frame>\n";
        }

        stream << "  </stack>\n";
    }

    stream << "</error>\n";

    return xml;
}

} // namespace XmlProtocol
} // namespace Valgrind
