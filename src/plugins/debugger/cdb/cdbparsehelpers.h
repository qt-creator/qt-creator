/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef CDBPARSEHELPERS_H
#define CDBPARSEHELPERS_H

#include "breakpoint.h"

#include <QtGlobal>
#include <QList>
#include <QVector>
#include <QPair>
#include <QByteArray>

QT_BEGIN_NAMESPACE
class QVariant;
class QDebug;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {
class BreakpointData;
class BreakpointParameters;
struct ThreadData;
class Register;
class GdbMi;
class DisassemblerLines;

// Perform mapping on parts of the source tree as reported by/passed to debugger
// in case the user has specified such mappings in the global settings.
enum SourcePathMode { DebuggerToSource, SourceToDebugger };
QString cdbSourcePathMapping(QString fileName,
                             const QList<QPair<QString, QString> > &sourcePathMapping,
                             SourcePathMode mode);

// Ensure unique 'namespace' for breakpoints of the breakhandler.
enum { cdbBreakPointStartId = 1000 };

int breakPointIdToCdbId(const BreakpointModelId &id);
BreakpointModelId cdbIdToBreakpointModelId(const GdbMi &id);
BreakpointResponseId cdbIdToBreakpointResponseId(const GdbMi &id);

// Convert breakpoint in CDB syntax (applying source path mappings using native paths).
QByteArray cdbAddBreakpointCommand(const BreakpointParameters &d,
                                   const QList<QPair<QString, QString> > &sourcePathMapping,
                                   BreakpointModelId id = BreakpointModelId(quint16(-1)), bool oneshot = false);
// Parse extension command listing breakpoints.
// Note that not all fields are returned, since file, line, function are encoded
// in the expression (that is in addition deleted on resolving for a bp-type breakpoint).
void parseBreakPoint(const GdbMi &gdbmi, BreakpointResponse *r, QString *expression = 0);

// Convert a CDB integer value: '00000000`0012a290' -> '12a290', '0n10' ->'10'
QByteArray fixCdbIntegerValue(QByteArray t, bool stripLeadingZeros = false, int *basePtr = 0);
// Convert a CDB integer value into quint64 or int64
QVariant cdbIntegerValue(const QByteArray &t);
// Write memory (f ...).
QByteArray cdbWriteMemoryCommand(quint64 addr, const QByteArray &data);

QString debugByteArray(const QByteArray &a);
QString StringFromBase64EncodedUtf16(const QByteArray &a);

DisassemblerLines parseCdbDisassembler(const QList<QByteArray> &a);

// Model EXCEPTION_RECORD + firstchance
struct WinException
{
    WinException();
    void fromGdbMI(const GdbMi &);
    QString toString(bool includeLocation = false) const;

    unsigned exceptionCode;
    unsigned exceptionFlags;
    quint64 exceptionAddress;
    quint64 info1;
    quint64 info2;
    bool firstChance;
    QByteArray file;
    int lineNumber;
    QByteArray function;
};

QDebug operator<<(QDebug s, const WinException &e);

} // namespace Internal
} // namespace Debugger

#endif // CDBPARSEHELPERS_H
