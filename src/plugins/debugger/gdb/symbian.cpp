/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "symbian.h"
#include "registerhandler.h"
#include "threadshandler.h"
#include <codautils.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextStream>
#include <QFileInfo>

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
    QTC_CHECK(false);
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
    return d << QString::fromLatin1("[%1,%2] (size %3) ")
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
    ba += Coda::hexxNumber(value);
    return ba;
}

///////////////////////////////////////////////////////////////////////////
//
// Thread
//
///////////////////////////////////////////////////////////////////////////

Thread::Thread(unsigned theId) : id(theId)
{
    resetRegisters();
}

void Thread::resetRegisters()
{
    qFill(registers, registers + RegisterCount, uint(0));
    registerValid = false;
}

QByteArray Thread::gdbReportRegisters() const
{
    QByteArray ba;
    for (int i = 0; i < 16; ++i) {
        const uint reg = Coda::swapEndian(registers[i]);
        ba += Coda::hexNumber(reg, 8);
    }
    return ba;
}

QByteArray Thread::registerContentsLogMessage() const
{
    QByteArray logMsg;
    for (int i = 0; i < RegisterCount; ++i) {
        logMsg += dumpRegister(i, registers[i]);
        logMsg += ' ';
    }
    return logMsg;
}

QByteArray Thread::gdbRegisterLogMessage(bool verbose) const
{
    QByteArray logMsg = "Register contents: (Thread 0x";
    logMsg += QByteArray::number(id, 16);
    logMsg += " ) ";
    if (verbose)
        logMsg += registerContentsLogMessage();
    return logMsg;
}

QByteArray Thread::gdbReportSingleRegister(unsigned i) const
{
    if (i == RegisterPSGdb)
        i = RegisterPSCoda;
    if (i >= RegisterCount)
        return QByteArray("0000"); // Unknown
    QByteArray ba;
    appendInt(&ba, registers[i], Coda::LittleEndian);
    return ba.toHex();
}

QByteArray Thread::gdbSingleRegisterLogMessage(unsigned i) const
{
    if (i == RegisterPSGdb)
        i = RegisterPSCoda;
    if (i >= RegisterCount)
        return QByteArray("Read single unknown register #") + QByteArray::number(i);
    QByteArray logMsg = "Read Register ";
    logMsg += dumpRegister(i, registers[i]);
    return logMsg;
}

///////////////////////////////////////////////////////////////////////////
//
// Snapshot
//
///////////////////////////////////////////////////////////////////////////

Snapshot::Snapshot()
{
    reset();
    threadInfo.reserve(10);
}

void Snapshot::reset()
{
    MEMORY_DEBUG("RESET SNAPSHOT MEMORY INITIALLY: " << memory.size() << " BLOCK LEFT");
    for (Memory::Iterator it = memory.begin(); it != memory.end(); ) {
        MEMORY_DEBUG("EXAMINING " << it.key());
        it = memory.erase(it);
    }
    MEMORY_DEBUG("RESET SNAPSHOT MEMORY FINALLY: " << memory.size() << " BLOCK LEFT");

    const int threadCount = threadInfo.size();
    for (int i =0; i < threadCount; i++) {
        threadInfo[i].resetRegisters();
        threadInfo[i].state.clear();
    }

    wantedMemory = MemoryRange();
    lineFromAddress = 0;
    lineToAddress = 0;
}

void Snapshot::resetMemory()
{
    memory.clear();
    reset();
}

void Snapshot::fullReset()
{
    threadInfo.clear();
    resetMemory();
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
    foreach(const Thread &thread, threadInfo) {
        str << " Thread " << thread.id << ' ' << thread.state
            << " Register valid " << thread.registerValid << ' ';
        if (thread.registerValid) {
            for (int i = 0; i < RegisterCount; i++) {
                if (i)
                    str << ", ";
                str << " R" << i << "=0x";
                str.setIntegerBase(16);
                str << thread.registers[i];
                str.setIntegerBase(10);
            }
        }
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

void Snapshot::addThread(uint id)
{
    if (!id || id == uint(-1)) {
        qWarning("Cowardly refusing to add thread %d", id);
        return;
    }

    const int index = indexOfThread(id);
    if (index == -1) {
        threadInfo.push_back(Thread(id));
    } else {
        threadInfo[index].resetRegisters();
        qWarning("Attempt to re-add existing thread %d", id);
    }
}

void Snapshot::removeThread(uint id)
{
    const int index = indexOfThread(id);
    if (index != -1) {
        threadInfo.remove(index);
    } else {
        qWarning("Attempt to remove non-existing thread %d", id);
    }
}

int Snapshot::indexOfThread(uint id) const
{
    const int count = threadInfo.size();
    for (int i = 0; i < count; i++)
        if (threadInfo.at(i).id == id)
            return i;
    return -1;
}

uint *Snapshot::registers(uint threadId)
{
    const int index = indexOfThread(threadId);
    QTC_ASSERT(index != -1, { qWarning("No such thread %d", threadId); return 0; } );
    return threadInfo[index].registers;
}

const uint *Snapshot::registers(uint threadId) const
{
    const int index = indexOfThread(threadId);
    QTC_ASSERT(index != -1, return 0; );
    return threadInfo.at(index).registers;
}

uint Snapshot::registerValue(uint threadId, uint index)
{
    if (const uint *regs = registers(threadId))
        return regs[index];
    return 0;
}

void Snapshot::setRegisterValue(uint threadId, uint index, uint value)
{
    uint *regs = registers(threadId);
    QTC_ASSERT(regs, return; );
    regs[index] = value;
}

bool Snapshot::registersValid(uint threadId) const
{
    const int index = indexOfThread(threadId);
    return index != -1 ? threadInfo.at(index).registerValid : false;
}

void Snapshot::setRegistersValid(uint threadId, bool e)
{
    const int index = indexOfThread(threadId);
    QTC_ASSERT(index != -1, return; );
    threadInfo[index].registerValid = e;
}

void Snapshot::setThreadState(uint threadId, const QString &state)
{
    const int index = indexOfThread(threadId);
    QTC_ASSERT(index != -1, return; );
    threadInfo[index].state = state;
}

QByteArray Snapshot::gdbQsThreadInfo() const
{
    // FIXME: Limit packet length by using qsThreadInfo packages ('m', ..'l')
    QByteArray response(1, 'l');
    const int count = threadInfo.size();
    for (int i = 0; i < count; i++) {
        if (i)
            response += ',';
        response += Coda::hexNumber(threadInfo.at(i).id);
    }
    return response;
}

// $qThreadExtraInfo,1f9#55
QByteArray Snapshot::gdbQThreadExtraInfo(const QByteArray &cmd) const
{
    const int pos = cmd.indexOf(',');
    if (pos != 1) {
        const uint threadId = cmd.mid(pos + 1).toUInt(0, 16);
        const int threadIndex = indexOfThread(threadId);
        if (threadIndex != -1 && !threadInfo.at(threadIndex).state.isEmpty())
            return threadInfo.at(threadIndex).state.toAscii().toHex();
    }
    return QByteArray("Nothing special").toHex();
}

static void gdbAppendRegister(QByteArray *ba, uint regno, uint value)
{
    ba->append(Coda::hexNumber(regno, 2));
    ba->append(':');
    ba->append(Coda::hexNumber(Coda::swapEndian(value), 8));
    ba->append(';');
}

QByteArray Snapshot::gdbStopMessage(uint threadId, int signalNumber, bool reportThreadId) const
{
    QByteArray ba = ('T' + Coda::hexNumber(signalNumber, 2));
    if (reportThreadId) {
        ba += "thread:";
        ba += Coda::hexNumber(threadId, 3);
        ba += ';';
    }
    const int threadIndex = indexOfThread(threadId);
    QTC_ASSERT(threadIndex != -1, return QByteArray(); );
    const Thread &thread = threadInfo.at(threadIndex);
    for (int i = 0; i < 16; ++i)
        gdbAppendRegister(&ba, i, thread.registers[i]);
    // FIXME: those are not understood by gdb 6.4
    //for (int i = 16; i < 25; ++i)
    //    appendRegister(&ba, i, 0x0);
    gdbAppendRegister(&ba, RegisterPSGdb, thread.registers[RegisterPSCoda]);
    return ba;
}

// Format log message for memory access with some smartness about registers
QByteArray Snapshot::memoryReadLogMessage(uint addr, uint threadId, bool verbose, const QByteArray &ba) const
{
    QByteArray logMsg = "memory contents";
    const uint *regs = registers(threadId);
    if (verbose && regs) {
        logMsg += " addr: " + Coda::hexxNumber(addr);
        // indicate dereferencing of registers
        if (ba.size() == 4) {
            if (addr == regs[RegisterPC]) {
                logMsg += "[PC]";
            } else if (addr == regs[RegisterPSCoda]) {
                logMsg += "[PSTrk]";
            } else if (addr == regs[RegisterSP]) {
                logMsg += "[SP]";
            } else if (addr == regs[RegisterLR]) {
                logMsg += "[LR]";
            } else if (addr > regs[RegisterSP] &&
                    (addr - regs[RegisterSP]) < 10240) {
                logMsg += "[SP+"; // Stack area ...stack seems to be top-down
                logMsg += QByteArray::number(addr - regs[RegisterSP]);
                logMsg += ']';
            }
        }
        logMsg += " length ";
        logMsg += QByteArray::number(ba.size());
        logMsg += " :";
        logMsg += Coda::stringFromArray(ba, ba.size()).toAscii();
    }
    return logMsg;
}

void Snapshot::syncRegisters(uint threadId, RegisterHandler *handler) const
{
    // Take advantage of direct access to cached register values.
    const int threadIndex = indexOfThread(threadId);
    QTC_ASSERT(threadIndex != -1, return ;);
    const Thread &thread = threadInfo.at(threadIndex);
    QTC_ASSERT(thread.registerValid, return ;);

    Registers debuggerRegisters = handler->registers();
    QTC_ASSERT(debuggerRegisters.size() >= RegisterPSGdb,
        qDebug() << "HAVE: " << debuggerRegisters.size(); return);

    for (int i = 0; i < RegisterCount; ++i) {
        const int gdbIndex = i == RegisterPSCoda ? int(RegisterPSGdb) : i;
        Register &reg = debuggerRegisters[gdbIndex];
        reg.value = Coda::hexxNumber(thread.registers[i]);
    }
    handler->setAndMarkRegisters(debuggerRegisters);
}

void Snapshot::parseGdbStepRange(const QByteArray &cmd, bool so)
{
    const int pos = cmd.indexOf(',', 8);
    lineFromAddress = cmd.mid(8, pos - 8).toUInt(0, 16);
    lineToAddress = cmd.mid(pos + 1).toUInt(0, 16);
    stepOver = so;
}

void Snapshot::syncThreads(ThreadsHandler *handler) const
{
    // Take advantage of direct access to cached register values.
    Threads threads;
    const unsigned count = threadInfo.size();
    for (unsigned t = 0; t < count; ++t) {
        ThreadData thread(t + 1); // Fake gdb thread ids starting from 1
            thread.targetId = QString::number(threadInfo.at(t).id);
            thread.state = threadInfo.at(t).state;
            threads.append(thread);
    }
    handler->setThreads(threads);
}

// Answer to gdb's 'qSupported' query:
// Increase buffer size for qXfer::libraries XML response
const char *gdbQSupported =
    "PacketSize=20000;"
    "QPassSignals+;"
    "QStartNoAckMode+;"
    "qXfer:libraries:read+;"
    // "qXfer:auxv:read+;"
    "qXfer:features:read+;"
    "qRelocInsn-"; // Relocate instructions for trace (gdb 7.2+): Not supported.

// Answer to gdb "qXfer:features:read:target.xml:" request
// "l<target><architecture>symbianelf</architecture></target>"
// "l<target><architecture>arm-none-symbianelf</architecture></target>"

const char *gdbArchitectureXml = "l<target><architecture>arm</architecture></target>";

QVector<QByteArray> gdbStartupSequence()
{
    QVector<QByteArray> s;
    s.reserve(10);
    s.push_back(QByteArray("set breakpoint always-inserted on"));
    s.push_back(QByteArray("set breakpoint auto-hw on"));
    s.push_back(QByteArray("set trust-readonly-sections on")); // No difference?
    s.push_back(QByteArray("set displaced-stepping on")); // No difference?
    s.push_back(QByteArray("set mem inaccessible-by-default"));
    s.push_back(QByteArray("mem 0x00400000 0x70000000 cache"));
    s.push_back(QByteArray("mem 0x70000000 0x80000000 cache ro"));
    // FIXME: replace with  stack-cache for newer gdb?
    s.push_back(QByteArray("set remotecache on"));  // "info dcache" to check
    return s;
}

// Local symbol file handling

enum { symDebug = 0 };

// Build complete file name of a local sym file from DLL
// 'QtCore.dll' to 'c:\\foo\QtCore.dll.sym'.

static inline QString symFileName(const QString &folder,
                                  const QString &libName)
{
    QString fileName = folder;
    fileName.append(QLatin1Char('/'));
    fileName.append(libName);
    fileName.append(QLatin1String(".sym"));
    return fileName;
}

// Look up in local symbol file matching remote library loaded in
// cache pointed to by environmentname or in standard location
// (next to application.sym file).
QString localSymFileForLibrary(const QByteArray &libName,
                               // urel/udeb: exe directory
                               const QString &standardSymDirectory)
{
    // Check
    const QByteArray envSymFileCacheDirectory = qgetenv("QTC_SYMBIAN_SYMBOLFILE_CACHE");
    if (envSymFileCacheDirectory.isEmpty() && standardSymDirectory.isEmpty())
        return QString();
    // Base name
    int lastSlashPos = libName.lastIndexOf('/');
    if (lastSlashPos == -1)
        lastSlashPos = libName.lastIndexOf('\\');
    const QString libBaseName = QString::fromLatin1(lastSlashPos != - 1 ? libName.mid(lastSlashPos + 1) : libName);
    // Check environment variable
    if (!envSymFileCacheDirectory.isEmpty()) {
        const QFileInfo envFi(symFileName(QString::fromLatin1(envSymFileCacheDirectory), libBaseName));
        if (symDebug)
            qDebug("SYM-ENV: %s exists %d\n", qPrintable(envFi.absoluteFilePath()), envFi.isFile());
        if (envFi.isFile())
            return envFi.absoluteFilePath();
    }
    // Check standard location
    if (!standardSymDirectory.isEmpty()) {
        const QFileInfo standardFi(symFileName(standardSymDirectory, libBaseName));
        if (symDebug)
            qDebug("SYM-STANDARD: %s exists %d\n", qPrintable(standardFi.absoluteFilePath()), standardFi.isFile());
        if (standardFi.isFile())
            return standardFi.absoluteFilePath();
    }
    return QString();
}

// Return a load command for a local symbol file for a library with address.
QByteArray symFileLoadCommand(const QString &symFileNameIn,
                              quint64 code, quint64 data)
{
    QByteArray symFileName = symFileNameIn.toLatin1();
    symFileName.replace('\\', '/'); // gdb wants forward slashes
    QByteArray command = "add-symbol-file \"";
    command += symFileName;
    command += "\" 0x";
    command += QByteArray::number(code, 16);
    if (data) {
        command += " -s .data 0x";
        command += QByteArray::number(data, 16);
    }
    return command;
}

QString msgLoadLocalSymFile(const QString &symFileName,
                            const QByteArray &libName, quint64 code)
{
    return QString::fromLatin1("Loading symbol file '%1' for '%2' at 0x%3").
            arg(symFileName, QString::fromLatin1(libName)).
            arg(code, 0, 16);
}

} // namespace Symbian

// Generic gdb server helpers: Read address/length off a memory
// command like 'm845,455','X845,455:'
QPair<quint64, unsigned> parseGdbReadMemoryRequest(const QByteArray &cmd)
{
    QPair<quint64, unsigned> rc(0, 0);
    const int pos = cmd.indexOf(',');
    if (pos == -1)
        return rc;
    bool ok;
    rc.first = cmd.mid(1, pos - 1).toULongLong(&ok, 16);
    if (!ok)
        return rc;
    const int colonPos = cmd.indexOf(':');
    if (colonPos == -1)
        rc.second = cmd.mid(pos + 1).toUInt(&ok, 16);
    else
        rc.second = cmd.mid(pos + 1, colonPos - pos - 1 ).toUInt(&ok, 16);
    if (!ok)
        rc.first = 0;
    return rc;
}

// Generic gdb server helpers: Parse 'register write' ('P') request
// return register number/value
QPair<uint, uint> parseGdbWriteRegisterWriteRequest(const QByteArray &cmd)
{
    const int pos = cmd.indexOf('=');
    const QByteArray regName = cmd.mid(1, pos - 1);
    const QByteArray valueName = cmd.mid(pos + 1);
    bool ok = false;
    const uint registerNumber = regName.toUInt(&ok, 16);
    const uint value = Coda::swapEndian(valueName.toUInt(&ok, 16));
    return QPair<uint, uint>(registerNumber, value);
}

// Generic gdb server helpers: Parse 'set breakpoint' ('Z0') request
// return address/length
QPair<quint64, unsigned> parseGdbSetBreakpointRequest(const QByteArray &cmd)
{
    // $Z0,786a4ccc,4#99
    const int pos = cmd.lastIndexOf(',');
    bool ok1 = false;
    bool ok2 = false;
    const quint64 addr = cmd.mid(3, pos - 3).toULongLong(&ok1, 16);
    const uint len = cmd.mid(pos + 1).toUInt(&ok2, 16);
    return ok1 && ok2 ? QPair<quint64, unsigned>(addr, len) : QPair<quint64, unsigned>(0, 0);
}

} // namespace Internal
} // namespace Debugger
