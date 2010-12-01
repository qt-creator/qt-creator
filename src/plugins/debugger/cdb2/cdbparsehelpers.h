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

#ifndef CDBPARSEHELPERS_H
#define CDBPARSEHELPERS_H

#include <QtCore/QtGlobal>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QByteArray>

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
} // namespace Internal

namespace Cdb {

// Convert breakpoint in CDB syntax.
QByteArray cdbAddBreakpointCommand(const Debugger::Internal::BreakpointParameters &d, bool oneshot = false, int id = -1);

// Convert a CDB integer value: '00000000`0012a290' -> '12a290', '0n10' ->'10'
QByteArray fixCdbIntegerValue(QByteArray t, bool stripLeadingZeros = false, int *basePtr = 0);
// Convert a CDB integer value into quint64 or int64
QVariant cdbIntegerValue(const QByteArray &t);

QString debugByteArray(const QByteArray &a);
QString StringFromBase64EncodedUtf16(const QByteArray &a);

// Model EXCEPTION_RECORD + firstchance
struct WinException
{
    WinException();
    void fromGdbMI(const Debugger::Internal::GdbMi &);
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

} // namespace Cdb
} // namespace Debugger

#endif // CDBPARSEHELPERS_H
