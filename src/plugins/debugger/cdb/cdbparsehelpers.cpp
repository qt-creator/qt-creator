/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbparsehelpers.h"
#include "breakpoint.h"
#include "threadshandler.h"
#include "registerhandler.h"
#include "bytearrayinputstream.h"
#include "gdb/gdbmi.h"
#ifdef Q_OS_WIN
#    include "shared/dbgwinutils.h"
#endif
#include <QtCore/QByteArray>
#include <QtCore/QVariant>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <utils/qtcassert.h>

#include <cctype>

namespace Debugger {
namespace Internal {

// Convert breakpoint in CDB syntax.
QByteArray cdbAddBreakpointCommand(const BreakpointParameters &bpIn,
                                   BreakpointId id /* = BreakpointId(-1) */,
                                   bool oneshot)
{
#ifdef Q_OS_WIN
    const BreakpointParameters bp = fixWinMSVCBreakpoint(bpIn);
#else
    const BreakpointParameters bp = bpIn;
#endif

    QByteArray rc;
    ByteArrayInputStream str(rc);

    if (bp.threadSpec >= 0)
        str << '~' << bp.threadSpec << ' ';

    // Currently use 'bu' so that the offset expression (including file name)
    // is kept when reporting back breakpoints (which is otherwise discarded when resolving).
    str << (bp.type == Watchpoint ? "ba" : "bu");
    if (id != BreakpointId(-1))
        str << id;
    str << ' ';
    if (oneshot)
        str << "/1 ";
    switch (bp.type) {
    case UnknownType:
    case BreakpointAtCatch:
    case BreakpointAtThrow:
    case BreakpointAtMain:
        QTC_ASSERT(false, return QByteArray(); )
        break;
    case BreakpointByAddress:
        str << hex << hexPrefixOn << bp.address << hexPrefixOff << dec;
        break;
    case BreakpointByFunction:
        if (!bp.module.isEmpty())
            str << bp.module << '!';
        str << bp.functionName;
        break;
    case BreakpointByFileAndLine:
        str << '`';
        if (!bp.module.isEmpty())
            str << bp.module << '!';
        str << QDir::toNativeSeparators(bp.fileName) << ':' << bp.lineNumber << '`';
        break;
    case Watchpoint:
        str << "rw 1 " << hex << hexPrefixOn << bp.address << hexPrefixOff << dec;
        break;
    }
    if (bp.ignoreCount)
        str << ' ' << (bp.ignoreCount + 1);
    // Condition currently unsupported.
    if (!bp.command.isEmpty())
        str << " \"" << bp.command << '"';
    return rc;
}

// Fix a CDB integer value: '00000000`0012a290' -> '12a290', '0n10' ->'10'
QByteArray fixCdbIntegerValue(QByteArray t, bool stripLeadingZeros, int *basePtr /* = 0 */)
{
    if (t.isEmpty())
        return t;
    int base = 16;
    // Prefixes
    if (t.startsWith("0x")) {
        t.remove(0, 2);
    } else if (t.startsWith("0n")) {
        base = 10;
        t.remove(0, 2);
    }
    if (base == 16 && t.size() >= 9 && t.at(8) == '`')
        t.remove(8, 1);
    if (stripLeadingZeros) { // Strip all but last '0'
        const int last = t.size() - 1;
        int pos = 0;
        for ( ; pos < last && t.at(pos) == '0'; pos++) ;
        if (pos)
            t.remove(0, pos);
    }
    if (basePtr)
        *basePtr = base;
    return t;
}

// Convert a CDB integer value: '00000000`0012a290' -> '12a290', '0n10' ->'10'
QVariant cdbIntegerValue(const QByteArray &t)
{
    int base;
    const QByteArray fixed = fixCdbIntegerValue(t, false, &base);
    bool ok;
    const QVariant converted = base == 16 ?
                               fixed.toULongLong(&ok, base) :
                               fixed.toLongLong(&ok, base);
    QTC_ASSERT(ok, return QVariant(); )
    return converted;
}

/* \code
0:002> ~ [Debugger-Id] Id: <hex pid> <hex tid> Suspends count thread environment block add state name
   0  Id: 133c.1374 Suspend: 1 Teb: 000007ff`fffdd000 Unfrozen
.  2  Id: 133c.1160 Suspend: 1 Teb: 000007ff`fffd9000 Unfrozen "QThread"
   3  Id: 133c.38c Suspend: 1 Teb: 000007ff`fffd7000 Unfrozen "QThread"
\endcode */

static inline bool parseThread(QByteArray line, ThreadData *thread, bool *current)
{
    *current = false;
    if (line.size() < 5)
        return false;
    *current = line.at(0) == '.';
    if (current)
        line[0] = ' ';
    const QList<QByteArray> tokens = simplify(line).split(' ');
    if (tokens.size() < 8 || tokens.at(1) != "Id:")
        return false;
    switch (tokens.size()) { // fallthru intended
    case 9:
        thread->name = QString::fromLocal8Bit(tokens.at(8));
    case 8:
        thread->state = QString::fromLocal8Bit(tokens.at(7));
    case 3: {
        const QByteArray &pidTid = tokens.at(2);
        const int dotPos = pidTid.indexOf('.');
        if (dotPos != -1)
            thread->targetId = QLatin1String("0x") + QString::fromAscii(pidTid.mid(dotPos + 1));
    }
    case 1:
        thread->id = tokens.at(0).toInt();
        break;
    } // switch size
    return true;
}

// Helper to retrieve an int child from GDBMI
static inline bool gdbmiChildToInt(const GdbMi &parent, const char *childName, int *target)
{
    const GdbMi childBA = parent.findChild(childName);
    if (childBA.isValid()) {
        bool ok;
        const int v = childBA.data().toInt(&ok);
        if (ok) {
            *target = v;
            return  true;
        }
    }
    return false;
}

// Helper to retrieve an bool child from GDBMI
static inline bool gdbmiChildToBool(const GdbMi &parent, const char *childName, bool *target)
{
    const GdbMi childBA = parent.findChild(childName);
    if (childBA.isValid()) {
        *target = childBA.data() == "true";
        return true;
    }
    return false;
}

// Parse extension command listing breakpoints.
// Note that not all fields are returned, since file, line, function are encoded
// in the expression (that is in addition deleted on resolving for a bp-type breakpoint).
BreakpointId parseBreakPoint(const GdbMi &gdbmi, BreakpointResponse *r,
                             QString *expression /*  = 0 */)
{
    BreakpointId id = BreakpointId(-1);
    gdbmiChildToInt(gdbmi, "number", &(r->number));
    gdbmiChildToBool(gdbmi, "enabled", &(r->enabled));
    gdbmiChildToBool(gdbmi, "deferred", &(r->pending));
    const GdbMi idG = gdbmi.findChild("id");
    if (idG.isValid()) { // Might not be valid if there is not id
        bool ok;
        const BreakpointId cid = idG.data().toULongLong(&ok);
        if (ok)
            id = cid;
    }
    const GdbMi moduleG = gdbmi.findChild("module");
    if (moduleG.isValid())
        r->module = QString::fromLocal8Bit(moduleG.data());
    if (expression) {
        const GdbMi expressionG = gdbmi.findChild("expression");
        if (expressionG.isValid())
            *expression = QString::fromLocal8Bit(expressionG.data());
    }
    const GdbMi addressG = gdbmi.findChild("address");
    if (addressG.isValid())
        r->address = addressG.data().toULongLong(0, 0);
    if (gdbmiChildToInt(gdbmi, "passcount", &(r->ignoreCount)))
        r->ignoreCount--;
    gdbmiChildToInt(gdbmi, "thread", &(r->threadSpec));
    return id;
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
            if (uc >=32 && uc < 128) {
                str << a.at(i);
            } else {
                str << '<' << unsigned(uc) << '>';
            }
            break;
        }
    }
    return rc;
}

QString StringFromBase64EncodedUtf16(const QByteArray &a)
{
    QByteArray utf16 = QByteArray::fromBase64(a);
    utf16.append('\0');
    utf16.append('\0');
    return QString::fromUtf16(reinterpret_cast<const unsigned short *>(utf16.constData()));
}

WinException::WinException() :
    exceptionCode(0), exceptionFlags(0), exceptionAddress(0),
    info1(0),info2(0), firstChance(false), lineNumber(0)
{
}

void WinException::fromGdbMI(const GdbMi &gdbmi)
{
    exceptionCode = gdbmi.findChild("exceptionCode").data().toUInt();
    exceptionFlags = gdbmi.findChild("exceptionFlags").data().toUInt();
    exceptionAddress = gdbmi.findChild("exceptionAddress").data().toULongLong();
    firstChance = gdbmi.findChild("firstChance").data() != "0";
    const GdbMi ginfo1 = gdbmi.findChild("exceptionInformation0");
    if (ginfo1.isValid()) {
        info1 = ginfo1.data().toULongLong();
        const GdbMi ginfo2  = gdbmi.findChild("exceptionInformation1");
        if (ginfo2.isValid())
            info2 = ginfo1.data().toULongLong();
    }
    const GdbMi gLineNumber = gdbmi.findChild("exceptionLine");
    if (gLineNumber.isValid()) {
        lineNumber = gLineNumber.data().toInt();
        file = gdbmi.findChild("exceptionFile").data();
    }
    function = gdbmi.findChild("exceptionFunction").data();
}

QString WinException::toString(bool includeLocation) const
{
    QString rc;
    QTextStream str(&rc);
#ifdef Q_OS_WIN
    formatWindowsException(exceptionCode, exceptionAddress,
                           exceptionFlags, info1, info2, str);
#endif
    if (includeLocation) {
        if (lineNumber) {
            str << " at " << QLatin1String(file) << ':' << lineNumber;
        } else {
            if (!function.isEmpty())
                str << " in " << QLatin1String(function);
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

} // namespace Internal
} // namespace Debugger
