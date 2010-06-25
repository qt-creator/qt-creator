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

#ifndef SYMBIANUTILS_H
#define SYMBIANUTILS_H

#include <QtCore/QMap>
#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

//#define DEBUG_MEMORY  1
#if DEBUG_MEMORY
#   define MEMORY_DEBUG(s) qDebug() << s
#else
#   define MEMORY_DEBUG(s)
#endif
#define MEMORY_DEBUGX(s) qDebug() << s

namespace Debugger {
namespace Internal {

struct GdbResult {
    QByteArray data;
};

struct MemoryRange
{
    MemoryRange() : from(0), to(0) {}
    MemoryRange(uint f, uint t);
    void operator-=(const MemoryRange &other);
    bool intersects(const MemoryRange &other) const;
    quint64 hash() const { return (quint64(from) << 32) + to; }
    bool operator==(const MemoryRange &other) const { return hash() == other.hash(); }
    bool operator<(const MemoryRange &other) const { return hash() < other.hash(); }
    uint size() const { return to - from; }

    uint from; // Inclusive.
    uint to;   // Exclusive.
};

QDebug operator<<(QDebug d, const MemoryRange &range);

namespace Symbian {

enum CodeMode
{
    ArmMode = 0,
    ThumbMode,
};

enum TargetConstants
{
    RegisterCount = 17,
    RegisterSP = 13, // Stack Pointer
    RegisterLR = 14, // Return address
    RegisterPC = 15, // Program counter
    RegisterPSGdb = 25, // gdb's view of the world
    RegisterPSTrk = 16, // TRK's view of the world

    MemoryChunkSize = 256
};

enum { KnownRegisters = RegisterPSGdb + 1};

const char *registerName(int i);
QByteArray dumpRegister(uint n, uint value);

inline bool isReadOnly(const MemoryRange &mr)
{
    return  mr.from >= 0x70000000 && mr.to < 0x80000000;
}

struct Snapshot
{
    Snapshot() { reset(); }

    void reset(); // Leaves read-only memory cache alive.
    void fullReset(); // Also removes read-only memory cache.
    void insertMemory(const MemoryRange &range, const QByteArray &ba);
    QString toString() const;

    uint registers[RegisterCount];
    bool registerValid;
    typedef QMap<MemoryRange, QByteArray> Memory;
    Memory memory;

    // Current state.
    MemoryRange wantedMemory;

    // For next step.
    uint lineFromAddress;
    uint lineToAddress;
    bool stepOver;
};

struct Breakpoint
{
    Breakpoint(uint offset_ = 0)
    {
        number = 0;
        offset = offset_;
        mode = ArmMode;
    }
    uint offset;
    ushort number;
    CodeMode mode;
};

} // namespace Symbian
} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryRange);

#endif // SYMBIANUTILS_H
