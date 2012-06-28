/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SYMBIANUTILS_H
#define SYMBIANUTILS_H

#include <QMap>
#include <QByteArray>
#include <QString>
#include <QMetaType>
#include <QVector>
#include <QPair>

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
class RegisterHandler;
class ThreadsHandler;
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

// Signals to be passed to gdb server as stop reason (2 digit hex)
enum GdbServerStopReason {
    gdbServerSignalTrap = 5,     // Trap/Breakpoint, etc.
    gdbServerSignalSegfault = 11 // Segfault
};

namespace Symbian {

enum CodeMode
{
    ArmMode = 0,
    ThumbMode
};

enum TargetConstants
{
    RegisterCount = 17,
    RegisterSP = 13, // Stack Pointer
    RegisterLR = 14, // Return address
    RegisterPC = 15, // Program counter
    RegisterPSGdb = 25, // gdb's view of the world
    RegisterPSCoda = 16, // CODA's view of the world

    MemoryChunkSize = 256
};

enum { KnownRegisters = RegisterPSGdb + 1};

const char *registerName(int i);
QByteArray dumpRegister(uint n, uint value);

inline bool isReadOnly(const MemoryRange &mr)
{
    return  mr.from >= 0x70000000 && mr.to < 0x80000000;
}

// Snapshot thread with cached registers
struct Thread {
    explicit Thread(unsigned id = 0);

    void resetRegisters();
    // Gdb helpers for reporting values
    QByteArray gdbReportRegisters() const;
    QByteArray registerContentsLogMessage() const;
    QByteArray gdbRegisterLogMessage(bool verbose) const;
    QByteArray gdbReportSingleRegister(unsigned i) const;
    QByteArray gdbSingleRegisterLogMessage(unsigned i) const;

    uint id;
    uint registers[RegisterCount];
    bool registerValid;
    QString state; // Stop reason, for qsThreadExtraInfo
};

struct Snapshot
{
    Snapshot();

    void reset(); // Leaves read-only memory cache and threads alive.
    void resetMemory(); // Completely clears memory, leaves threads alive.
    void fullReset(); // Clear everything.
    void insertMemory(const MemoryRange &range, const QByteArray &ba);
    QString toString() const;

    // Helpers to format gdb query packets
    QByteArray gdbQsThreadInfo() const;
    QByteArray gdbQThreadExtraInfo(const QByteArray &cmd) const;
    // Format a gdb T05 stop message with thread and register set
    QByteArray gdbStopMessage(uint threadId, int signalNumber, bool reportThreadId) const;
    // Format a log message for memory access with some smartness about registers
    QByteArray memoryReadLogMessage(uint addr, uint threadId, bool verbose, const QByteArray &ba) const;
    // Gdb command parse helpers: 'salnext'
    void parseGdbStepRange(const QByteArray &cmd, bool stepOver);

    void addThread(uint threadId);
    void removeThread(uint threadId);
    int indexOfThread(uint threadId) const;
    // Access registers by thread
    const uint *registers(uint threadId) const;
    uint *registers(uint threadId);
    uint registerValue(uint threadId, uint index);
    void setRegisterValue(uint threadId, uint index, uint value);
    bool registersValid(uint threadId) const;
    void setRegistersValid(uint threadId, bool e);
    void setThreadState(uint threadId, const QString&);

    // Debugger view helpers: Synchronize registers of thread with register handler.
    void syncRegisters(uint threadId, RegisterHandler *handler) const;
    // Debugger view helpers: Synchronize threads with threads handler.
    void syncThreads(ThreadsHandler *handler) const;

    QVector<Thread> threadInfo;

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

// Gdb helpers
extern const char *gdbQSupported;
extern const char *gdbArchitectureXml;

QVector<QByteArray> gdbStartupSequence();

// Look up in symbol file matching library name in local cache
QString localSymFileForLibrary(const QByteArray &libName,
                               const QString &standardSymDirectory = QString());
// Return a load command for a local symbol file for a library
QByteArray symFileLoadCommand(const QString &symFileName, quint64 code,
                              quint64 data = 0);
// Utility message
QString msgLoadLocalSymFile(const QString &symFileName,
                            const QByteArray &libName, quint64 code);

} // namespace Symbian

// Generic gdb server helpers: read 'm','X' commands.
QPair<quint64, unsigned> parseGdbReadMemoryRequest(const QByteArray &cmd);
// Parse 'register write' ('P') request, return register number/value
QPair<uint, uint> parseGdbWriteRegisterWriteRequest(const QByteArray &cmd);
// Parse 'set breakpoint' ('Z0') request, return address/length
QPair<quint64, unsigned> parseGdbSetBreakpointRequest(const QByteArray &cmd);

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::MemoryRange)

#endif // SYMBIANUTILS_H
