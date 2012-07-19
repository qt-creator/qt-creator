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
    //DisassemblerLine(const QString &unparsed);
public:
    DisassemblerLine() : address(0), offset(0), lineNumber(0) {}
    bool isAssembler() const { return address != 0; }
    bool isCode() const { return lineNumber != 0; }
    bool isComment() const { return lineNumber == 0 && address == 0; }
    QString toString() const;
    void fromString(const QString &unparsed);

    // Return address of an assembly line "0x0dfd  bla".
    static quint64 addressFromDisassemblyLine(const QString &line);

public:
    quint64 address;  // (ass) Address of instruction in memory/in binary.
    QString function; // (ass) Function to which current instruction belongs.
    uint offset;      // (ass) Offset of instruction in relation to current function.
    uint lineNumber;  // (src) Line number in source.
    QByteArray rawData;  // (ass) Raw bytes of the instruction
    QString data;     // (ass) Instruction text, (src) source text, (cmt) arbitrary.
};

class DisassemblerLines
{
public:
    DisassemblerLines() {}

    bool coversAddress(quint64 address) const;
    void appendUnparsed(const QString &line);
    void appendLine(const DisassemblerLine &dl);
    // Mixed source/assembly: Retrieve contents of source (cached)
    void appendSourceLine(const QString &fileName, uint line);

    int size() const { return m_data.size(); }
    const DisassemblerLine &at(int i) const { return m_data.at(i); }
    int lineForAddress(quint64 address) const;

    quint64 startAddress() const;
    quint64 endAddress() const;

private:
    QString m_lastFunction;
    QVector<DisassemblerLine> m_data;
    QHash<quint64, int> m_rowCache;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DISASSEMBLERLINES_H
