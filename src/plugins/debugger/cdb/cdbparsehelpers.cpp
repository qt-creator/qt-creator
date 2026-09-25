// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cdbparsehelpers.h"

#include "stringinputstream.h"

#include <debugger/breakhandler.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/disassemblerlines.h>
#include <debugger/shared/hostutils.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QTextStream>
#include <QDebug>

enum { debugDisAsm = 0 };

namespace Debugger::Internal {

static QString withSlashes(QString path)
{
    return path.replace('\\', '/');
}

// Perform mapping on parts of the source tree as reported by/passed to debugger
// in case the user has specified such mappings in the global settings.
// That is, when debugging an executable built from 'X:\buildsrv\foo.cpp' and using a local
// source tree under 'c:\src', the user would specify a mapping 'X:\buildsrv'->'c:\src'
// and file names passed to breakpoints and reported stack traces can be converted.
QString cdbSourcePathMapping(QString fileName,
                             const QList<QPair<QString, QString> > &sourcePathMapping,
                             SourcePathMode mode)
{
    using SourcePathMapping = QPair<QString, QString>;

    if (fileName.isEmpty() || sourcePathMapping.isEmpty())
        return fileName;
    const QString comparedFileName = withSlashes(fileName);
    for (const SourcePathMapping &m : sourcePathMapping) {
        const QString source = withSlashes(mode == DebuggerToSource ? m.first : m.second);
        const int sourceSize = source.size();
        // Map parts of the path and ensure a slash follows.
        if (comparedFileName.size() > sourceSize
            && comparedFileName.startsWith(source, Qt::CaseInsensitive)) {
            if (comparedFileName.at(sourceSize) == '/') {
                const QString &target = mode == DebuggerToSource ? m.second: m.first;
                fileName.replace(0, sourceSize, target);
                return fileName;
            }
        }
    }
    return fileName;
}

// Determine file name to be used for breakpoints. Unless short path is set, perform reverse
// lookup in the source path mappings and convert to the native form of the file's own device.
static inline QString cdbBreakPointFileName(const BreakpointParameters &params,
                                            const QList<QPair<QString, QString> > &sourcePathMapping)
{
    if (params.fileName.isEmpty())
        return {};
    if (params.pathUsage == BreakpointUseShortPath)
        return params.fileName.fileName();
    const QString mapped = cdbSourcePathMapping(params.fileName.path(), sourcePathMapping,
                                                SourceToDebugger);
    return params.fileName.withNewPath(mapped).nativePath();
}

static BreakpointParameters fixWinMSVCBreakpoint(const BreakpointParameters &p)
{
    switch (p.type) {
    case UnknownBreakpointType:
    case LastBreakpointType:
    case BreakpointByFileAndLine:
    case BreakpointByFunction:
    case BreakpointByAddress:
    case BreakpointAtFork:
    case WatchpointAtExpression:
    case BreakpointAtSysCall:
    case WatchpointAtAddress:
    case BreakpointOnQmlSignalEmit:
    case BreakpointAtJavaScriptThrow:
        break;
    case BreakpointAtExec: { // Emulate by breaking on CreateProcessW().
        BreakpointParameters rc(BreakpointByFunction);
        rc.module = "kernel32";
        rc.functionName = "CreateProcessW";
        return rc;
    }
    case BreakpointAtThrow: {
        BreakpointParameters rc(BreakpointByFunction);
        rc.functionName = "CxxThrowException"; // MSVC runtime. Potentially ambiguous.
        return rc;
    }
    case BreakpointAtCatch: {
        BreakpointParameters rc(BreakpointByFunction);
        rc.functionName = "__CxxCallCatchBlock"; // MSVC runtime. Potentially ambiguous.
        return rc;
    }
    case BreakpointAtMain: {
        BreakpointParameters rc(BreakpointByFunction);
        rc.functionName = "main";
        rc.module = p.module;
        rc.oneShot = true;
        return rc;
    }
    } // switch
    return p;
}

QString breakPointCdbId(const Breakpoint &bp)
{
    static int bpId = 1;
    if (!bp->responseId().isEmpty())
        return bp->responseId();
    return QString::number(cdbBreakPointStartId + (bpId++) * cdbBreakPointIdMinorPart);
}

QString cdbAddBreakpointCommand(const BreakpointParameters &bpIn,
                                const QList<QPair<QString, QString> > &sourcePathMapping,
                                const QString &responseId)
{
    const BreakpointParameters params = fixWinMSVCBreakpoint(bpIn);
    QString rc;
    StringInputStream str(rc);

    if (params.threadSpec >= 0)
        str << '~' << params.threadSpec << ' ';

    // Currently use 'bu' so that the offset expression (including file name)
    // is kept when reporting back breakpoints (which is otherwise discarded
    // when resolving).
    str << (params.type == WatchpointAtAddress ? "ba" : "bu")
        << responseId
        << ' ';
    if (params.oneShot)
        str << "/1 ";
    switch (params.type) {
    case BreakpointAtFork:
    case BreakpointAtExec:
    case WatchpointAtExpression:
    case BreakpointAtSysCall:
    case BreakpointAtCatch:
    case BreakpointAtThrow:
    case BreakpointAtMain:
    case BreakpointOnQmlSignalEmit:
    case BreakpointAtJavaScriptThrow:
    case UnknownBreakpointType:
    case LastBreakpointType:
        QTC_ASSERT(false, return QString());
        break;
    case BreakpointByAddress:
        str << hex << hexPrefixOn << params.address << hexPrefixOff << dec;
        break;
    case BreakpointByFunction:
        if (!params.module.isEmpty())
            str << params.module << '!';
        str << params.functionName;
        break;
    case BreakpointByFileAndLine:
        str << '`';
        if (!params.module.isEmpty())
            str << params.module << '!';
        str << cdbBreakPointFileName(params, sourcePathMapping)
            << ':' << params.textPosition.line << '`';
        break;
    case WatchpointAtAddress: { // Read/write, no space here
        const unsigned size = params.size ? params.size : 1;
        str << 'r' << size << ' ' << hex << hexPrefixOn << params.address << hexPrefixOff << dec;
    }
        break;
    }
    if (params.ignoreCount)
        str << " 0n" << (params.ignoreCount + 1);
    // Condition currently unsupported.
    if (!params.command.isEmpty())
        str << " \"" << params.command << '"';
    return rc;
}

QString cdbClearBreakpointCommand(const Breakpoint &bp)
{
// FIME: Check
//    const int firstBreakPoint = breakPointCdbId(id);
//    if (id.isMinor())
//        return "bc " + QString::number(firstBreakPoint);
    // If this is a major break point we also want to delete all sub break points
    const int firstBreakPoint = bp->responseId().toInt();
    const int lastBreakPoint = firstBreakPoint + cdbBreakPointIdMinorPart - 1;
    return "bc " + QString::number(firstBreakPoint) + '-' + QString::number(lastBreakPoint);
}

QString cdbWriteMemoryCommand(quint64 addr, const QByteArray &data)
{
    QString cmd;
    StringInputStream str(cmd);
    str.setIntegerBase(16);
    str << "f " << addr << " L" << data.size();
    const int count = data.size();
    for (int i = 0 ; i < count ; i++ ) {
        const unsigned char uc = (unsigned char)data.at(i);
        str << ' ' << unsigned(uc);
    }
    return cmd;
}

QString debugByteArray(const QByteArray &a)
{
    QString rc;
    const int size = a.size();
    rc.reserve(size * 2);
    QTextStream str(&rc);
    for (int i = 0; i < size; i++) {
        const unsigned char uc = (unsigned char)(a.at(i));
        switch (uc) {
        case 0:
            str << "\\0";
            break;
        case '\n':
            str << "\\n";
            break;
        case '\t':
            str << "\\t";
            break;
        case '\r':
            str << "\\r";
            break;
        default:
            if (uc >=32 && uc < 128)
                str << a.at(i);
            else
                str << '<' << unsigned(uc) << '>';
            break;
        }
    }
    return rc;
}

WinException::WinException() = default;

void WinException::fromGdbMI(const GdbMi &gdbmi)
{
    exceptionCode = gdbmi["exceptionCode"].data().toUInt();
    exceptionFlags = gdbmi["exceptionFlags"].data().toUInt();
    exceptionAddress = gdbmi["exceptionAddress"].data().toULongLong();
    firstChance = gdbmi["firstChance"].data() != "0";
    const GdbMi ginfo1 = gdbmi["exceptionInformation0"];
    if (ginfo1.isValid()) {
        info1 = ginfo1.data().toULongLong();
        const GdbMi ginfo2  = gdbmi["exceptionInformation1"];
        if (ginfo2.isValid())
            info2 = ginfo2.data().toULongLong();
    }
    const GdbMi gLineNumber = gdbmi["exceptionLine"];
    if (gLineNumber.isValid()) {
        lineNumber = gLineNumber.toInt();
        file = gdbmi["exceptionFile"].data();
    }
    function = gdbmi["exceptionFunction"].data();
}

QString WinException::toString(bool includeLocation) const
{
    QString rc;
    QTextStream str(&rc);
    formatWindowsException(exceptionCode, exceptionAddress,
                           exceptionFlags, info1, info2, str);
    if (firstChance)
        str << " (first chance)";
    if (includeLocation) {
        if (lineNumber) {
            str << " at " << file << ':' << lineNumber;
        } else {
            if (!function.isEmpty())
                str << " in " << function;
        }
    }
    return rc;
}

QDebug operator<<(QDebug s, const WinException &e)
{
    QDebug nsp = s.nospace();
    nsp << "code=" << e.exceptionCode << ",flags=" << e.exceptionFlags
        << ",address=0x" << QString::number(e.exceptionAddress, 16)
        << ",firstChance=" << e.firstChance;
    return s;
}

/*!
    \fn DisassemblerLines Debugger::Internal::parseCdbDisassembler(const QList<QByteArray> &a)

    Parses CDB disassembler output into DisassemblerLines (with helpers).

    Expected options (prepend source file line):
    \code
    .asm source_line
    .lines
    \endcode

    should cause the 'u' command to produce:

    \code
gitgui!Foo::MainWindow::on_actionPtrs_triggered+0x1f9 [c:\qt\projects\gitgui\app\mainwindow.cpp @ 758]:
  225 00000001`3fcebfe9 488b842410050000 mov     rax,qword ptr [rsp+510h]
  225 00000001`3fcebff1 8b4030          mov     eax,dword ptr [rax+30h]
  226 00000001`3fcebff4 ffc0            inc     eax
      00000001`3fcebff6 488b8c2410050000 mov     rcx,qword ptr [rsp+510h]
...
QtCored4!QTextStreamPrivate::putString+0x34:
   10 00000000`6e5e7f64 90              nop
...
\endcode

    The algorithm checks for a function line and grabs the function name, offset and (optional)
    source file from it.
    Instruction lines are checked for address and source line number.
    When the source line changes, the source instruction is inserted.
*/

// Parse a function header line: Match: 'nsp::foo+0x<offset> [<file> @ <line>]:'
// or 'nsp::foo+0x<offset>:', 'nsp::foo [<file> @ <line>]:'
// Do not use regexp here as it is hard for functions like operator+, operator[].
static bool parseCdbDisassemblerFunctionLine(const QString &l,
                                      QString *currentFunction, quint64 *functionOffset,
                                      QString *sourceFile)
{
    if (l.isEmpty() || !l.endsWith(':') || l.at(0).isDigit() || l.at(0).isSpace())
        return false;
    int functionEnd = l.indexOf(' ');
    if (functionEnd < 0)
        functionEnd = l.size() - 1; // Nothing at all, just ':'
    const int offsetPos = l.indexOf("+0x");
    if (offsetPos > 0) {
        *currentFunction = l.left(offsetPos);
        *functionOffset = l.mid(offsetPos + 3, functionEnd - offsetPos - 3).trimmed().toULongLong(nullptr, 16);
    } else { // No offset, directly at beginning.
        *currentFunction = l.left(functionEnd);
        *functionOffset = 0;
    }
    sourceFile->clear();
    // Parse file and line.
    const int filePos = l.indexOf('[', functionEnd);
    if (filePos == -1)
        return true; // No file
    const int linePos = l.indexOf(" @ ", filePos + 1);
    if (linePos == -1)
        return false;
    *sourceFile = l.mid(filePos + 1, linePos - filePos - 1).trimmed();
    if (debugDisAsm)
        qDebug() << "Function with source: " << l << currentFunction
                 << functionOffset << sourceFile;
    return true;
}

static bool isHexToken(const QString &token)
{
    if (token.isEmpty())
        return false;
    for (const QChar c : token) {
        if (!c.isDigit() && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
            return false;
    }
    return true;
}

/* Parse an instruction line. "u" prints one source-line column in front of the
 * address, "uf" two; the address itself is a run of hex split by a backtick into
 * its high and low halves on 64 bit:
 *  '   21 00000001`3fcebff1 8b4030          mov     eax,dword ptr [rax+30h]'
 *  '   21    21 00000001`3fcebff1 8b4030    mov     eax,dword ptr [rax+30h]'
 * so the address is found rather than sat at a fixed column: it is the first
 * token that is either backtick-joined or eight or more hex digits wide. The
 * numeric column right before it, if any, is the source line, the one after it
 * the raw bytes, and the rest the instruction. */

static bool parseCdbDisassemblerLine(const QString &line, DisassemblerLine *dLine, uint *sourceLine)
{
    *sourceLine = 0;
    const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 3)
        return false;
    int addressIndex = -1;
    for (int i = 0; i < parts.size(); ++i) {
        const QString &part = parts.at(i);
        if (part.contains('`') || (part.size() >= 8 && isHexToken(part))) {
            addressIndex = i;
            break;
        }
    }
    if (addressIndex < 0 || addressIndex + 1 >= parts.size())
        return false;
    if (addressIndex >= 1) {
        bool lineOk = false;
        const uint parsed = parts.at(addressIndex - 1).toUInt(&lineOk);
        if (lineOk)
            *sourceLine = parsed;
    }
    QString addressS = parts.at(addressIndex);
    addressS.remove('`');
    bool ok = false;
    dLine->address = addressS.toULongLong(&ok, 16);
    if (!ok)
        return false;
    const QString rawData = parts.at(addressIndex + 1);
    if (!isHexToken(rawData))
        return false;
    const int addressPos = line.indexOf(parts.at(addressIndex));
    const int rawDataPos = line.indexOf(rawData, addressPos + parts.at(addressIndex).size());
    dLine->rawData = QByteArray::fromHex(rawData.toLatin1());
    dLine->bytes = rawData;
    dLine->data = line.mid(rawDataPos + rawData.size()).trimmed();
    return true;
}

DisassemblerLines parseCdbDisassembler(const QString &a)
{
    DisassemblerLines result;
    quint64 functionAddress = 0;
    uint lastSourceLine = 0;
    QString currentFunction;
    quint64 functionOffset = 0;
    QString sourceFile;

    const QStringList lines = a.split('\n');
    for (const QString &line : lines) {
        // New function. Append as comment line.
        if (parseCdbDisassemblerFunctionLine(line, &currentFunction, &functionOffset, &sourceFile)) {
            functionAddress = 0;
            DisassemblerLine commentLine;
            commentLine.data = line;
            result.appendLine(commentLine);
        } else {
            DisassemblerLine disassemblyLine;
            uint sourceLine;
            if (parseCdbDisassemblerLine(line, &disassemblyLine, &sourceLine)) {
                // New source line: Add source code if available.
                if (sourceLine && sourceLine != lastSourceLine) {
                    lastSourceLine = sourceLine;
                    result.appendSourceLine(sourceFile, sourceLine);
                }
            } else {
                qWarning("Unable to parse assembly line '%s'", qPrintable(line));
                disassemblyLine.fromString(line);
            }
            // Determine address of function from the first assembler line after a
            // function header line.
            if (!functionAddress && disassemblyLine.address)
                functionAddress = disassemblyLine.address - functionOffset;
            if (functionAddress && disassemblyLine.address)
                disassemblyLine.offset = disassemblyLine.address - functionAddress;
            disassemblyLine.function = currentFunction;
            result.appendLine(disassemblyLine);
        }
    }
    return result;
}

} // namespace Debugger::Internal
