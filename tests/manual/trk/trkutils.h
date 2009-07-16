/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_TRK_UTILS
#define DEBUGGER_TRK_UTILS

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QString>

namespace trk {

typedef unsigned char byte;

QByteArray decode7d(const QByteArray &ba);
QByteArray encode7d(const QByteArray &ba);

ushort extractShort(const char *data);
uint extractInt(const char *data);

QString quoteUnprintableLatin1(const QByteArray &ba);

// produces "xx "
QString stringFromByte(byte c);
// produces "xx xx xx "
QString stringFromArray(const QByteArray &ba);

QByteArray formatByte(byte b);
QByteArray formatShort(ushort s);
QByteArray formatInt(uint i);
QByteArray formatString(const QByteArray &str);

enum CodeMode
{
    ArmMode = 0,
    ThumbMode,
};

struct Session
{
    Session() {
        cpuMajor = 0;
        cpuMinor = 0;
        bigEndian = 0;
        defaultTypeSize = 0;
        fpTypeSize = 0;
        extended1TypeSize = 0;
        extended2TypeSize = 0;
        pid = 0;
        tid = 0;
        codeseg = 0;
        dataseg = 0;
    }

    byte cpuMajor;
    byte cpuMinor;
    byte bigEndian;
    byte defaultTypeSize;
    byte fpTypeSize;
    byte extended1TypeSize;
    byte extended2TypeSize;
    uint pid;
    uint tid;
    uint codeseg;
    uint dataseg;
    QHash<uint, uint> tokenToBreakpointIndex;
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

struct TrkResult
{
    TrkResult() { code = token = 0; }
    QString toString() const;

    byte code;
    byte token;
    QByteArray data;
};

// returns a QByteArray containing 0x01 0x90 <len> 0x7e encoded7d(ba) 0x7e
QByteArray frameMessage(byte command, byte token, const QByteArray &data);
TrkResult extractResult(QByteArray *buffer);
QByteArray errorMessage(byte code);

} // namespace trk

#endif // DEBUGGER_TRK_UTILS
