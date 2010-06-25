/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "symbian.h"
#include <trkutils.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QTextStream>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////////
//
// MemoryRange
//
///////////////////////////////////////////////////////////////////////////

MemoryRange::MemoryRange(uint f, uint t)
    : from(f), to(t)
{
    QTC_ASSERT(f <= t, qDebug() << "F: " << f << " T: " << t);
}

bool MemoryRange::intersects(const MemoryRange &other) const
{
    Q_UNUSED(other);
    QTC_ASSERT(false, /**/);
    return false; // FIXME
}

void MemoryRange::operator-=(const MemoryRange &other)
{
    if (from == 0 && to == 0)
        return;
    MEMORY_DEBUG("      SUB: "  << *this << " - " << other);
    if (other.from <= from && to <= other.to) {
        from = to = 0;
        return;
    }
    if (other.from <= from && other.to <= to) {
        from = qMax(from, other.to);
        return;
    }
    if (from <= other.from && to <= other.to) {
        to = qMin(other.from, to);
        return;
    }
    // This would split the range.
    QTC_ASSERT(false, qDebug() << "Memory::operator-() not handled for: "
        << *this << " - " << other);
}

QDebug operator<<(QDebug d, const MemoryRange &range)
{
    return d << QString("[%1,%2] (size %3) ")
        .arg(range.from, 0, 16).arg(range.to, 0, 16).arg(range.size());
}

namespace Symbian {

static const char *registerNames[KnownRegisters] =
{
    "A1", "A2", "A3", "A4",
    0, 0, 0, 0,
    0, 0, 0, "AP",
    "IP", "SP", "LR", "PC",
    "PSTrk", 0, 0, 0,
    0, 0, 0, 0,
    0, "PSGdb"
};

const char *registerName(int i)
{
    return registerNames[i];
}

QByteArray dumpRegister(uint n, uint value)
{
    QByteArray ba;
    ba += ' ';
    if (n < KnownRegisters && registerNames[n]) {
        ba += registerNames[n];
    } else {
        ba += '#';
        ba += QByteArray::number(n);
    }
    ba += '=';
    ba += trk::hexxNumber(value);
    return ba;
}


///////////////////////////////////////////////////////////////////////////
//
// Snapshot
//
///////////////////////////////////////////////////////////////////////////

void Snapshot::reset()
{
    for (Memory::Iterator it = memory.begin(); it != memory.end(); ++it) {
        if (isReadOnly(it.key())) {
            MEMORY_DEBUG("KEEPING READ-ONLY RANGE" << it.key());
        } else {
            it = memory.erase(it);
        }
    }
    for (int i = 0; i < RegisterCount; ++i)
        registers[i] = 0;
    registerValid = false;
    wantedMemory = MemoryRange();
    lineFromAddress = 0;
    lineToAddress = 0;
}

void Snapshot::fullReset()
{
    memory.clear();
    reset();
}

void Snapshot::insertMemory(const MemoryRange &range, const QByteArray &ba)
{
    QTC_ASSERT(range.size() == uint(ba.size()),
        qDebug() << "RANGE: " << range << " BA SIZE: " << ba.size(); return);

    MEMORY_DEBUG("INSERT: " << range);
    // Try to combine with existing chunk.
    Snapshot::Memory::iterator it = memory.begin();
    Snapshot::Memory::iterator et = memory.end();
    for ( ; it != et; ++it) {
        if (range.from == it.key().to) {
            MEMORY_DEBUG("COMBINING " << it.key() << " AND " << range);
            QByteArray data = *it;
            data.append(ba);
            const MemoryRange res(it.key().from, range.to);
            memory.remove(it.key());
            MEMORY_DEBUG(" TO(1)  " << res);
            insertMemory(res, data);
            return;
        }
        if (it.key().from == range.to) {
            MEMORY_DEBUG("COMBINING " << range << " AND " << it.key());
            QByteArray data = ba;
            data.append(*it);
            const MemoryRange res(range.from, it.key().to);
            memory.remove(it.key());
            MEMORY_DEBUG(" TO(2)  " << res);
            insertMemory(res, data);
            return;
        }
    }

    // Not combinable, add chunk.
    memory.insert(range, ba);
}

QString Snapshot::toString() const
{
    typedef QMap<MemoryRange, QByteArray>::const_iterator MemCacheConstIt;
    QString rc;
    QTextStream str(&rc);
    str << "Register valid " << registerValid << ' ';
    for (int i = 0; i < RegisterCount; i++) {
        if (i)
            str << ", ";
        str << " R" << i << "=0x";
        str.setIntegerBase(16);
        str << registers[i];
        str.setIntegerBase(10);
    }
    str << '\n';
    // For next step.
    if (!memory.isEmpty()) {
        str.setIntegerBase(16);
        str << "Memory:\n";
        const MemCacheConstIt mcend = memory.constEnd();
        for (MemCacheConstIt it = memory.constBegin(); it != mcend; ++it)
            str << "  0x" << it.key().from << " - 0x" << it.key().to << '\n';
    }
    return rc;
}

} // namespace Symbian
} // namespace Internal
} // namespace Debugger
