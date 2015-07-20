/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_DISASSEMBLERLINES_H
#define DEBUGGER_DISASSEMBLERLINES_H

#include <QStringList>
#include <QHash>
#include <QVector>

namespace Debugger {
namespace Internal {

// A DisassemblerLine represents either
//  - an assembler instruction (address, offset, function, data fields), or
//  - a code line (lineNumber, data fields), or
//  - a comment line.

class DisassemblerLine
{
public:
    DisassemblerLine() : address(0), offset(0), lineNumber(0), hunk(0) {}
    bool isAssembler() const { return address != 0; }
    bool isCode() const { return lineNumber != 0; }
    bool isComment() const { return lineNumber == 0 && address == 0; }
    QString toString(int maxOp = 0) const;
    void fromString(const QString &unparsed);

public:
    quint64 address;  // (ass) Address of instruction in memory/in binary.
    QString function; // (ass) Function to which current instruction belongs.
    uint offset;      // (ass) Offset of instruction in relation to current function.
    uint lineNumber;  // (src) Line number in source.
    uint hunk;        // (src) Number of hunk if source line was split
    QByteArray rawData;  // (ass) Raw bytes of the instruction
    QString data;     // (ass) Instruction text, (src) source text, (cmt) arbitrary.
    QString bytes;    // (ass) The instruction in raw bytes
};

class DisassemblerLines
{
public:
    DisassemblerLines() : m_bytesLength(0) {}

    bool coversAddress(quint64 address) const;
    void appendUnparsed(const QString &line);
    void appendLine(const DisassemblerLine &dl);
    void appendComment(const QString &line);
    // Mixed source/assembly: Retrieve contents of source (cached)
    void appendSourceLine(const QString &fileName, uint line);
    QString toString() const;
    void setBytesLength(int x) { m_bytesLength = x; }
    int bytesLength() const  { return m_bytesLength; }

    int size() const { return m_data.size(); }
    const DisassemblerLine &at(int i) const { return m_data.at(i); }
    int lineForAddress(quint64 address) const;
    QVector<DisassemblerLine> data() const { return m_data; }

    quint64 startAddress() const;
    quint64 endAddress() const;

private:
    QString m_lastFunction;
    int m_bytesLength;
    QVector<DisassemblerLine> m_data;
    QHash<quint64, int> m_rowCache;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DISASSEMBLERLINES_H
