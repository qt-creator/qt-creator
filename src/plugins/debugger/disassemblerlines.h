// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>
#include <QHash>
#include <QVector>

namespace Debugger::Internal {

// A DisassemblerLine represents either
//  - an assembler instruction (address, offset, function, data fields), or
//  - a code line (lineNumber, data fields), or
//  - a comment line.

class DisassemblerLine
{
public:
    DisassemblerLine() = default;
    bool isAssembler() const { return address != 0; }
    bool isCode() const { return lineNumber != 0; }
    bool isComment() const { return lineNumber == 0 && address == 0; }
    QString toString(int maxOp = 0) const;
    void fromString(const QString &unparsed);

public:
    quint64 address = 0;    // (ass) Address of instruction in memory/in binary.
    QString function;       // (ass) Function to which current instruction belongs.
    QString fileName;       // (src) Source file
    uint offset = 0;        // (ass) Offset of instruction in relation to current function.
    int lineNumber = 0;     // (src) Line number in source.
    uint hunk = 0;          // (src) Number of hunk if source line was split
    QByteArray rawData;     // (ass) Raw bytes of the instruction
    QString data;           // (ass) Instruction text, (src) source text, (cmt) arbitrary.
    QString bytes;          // (ass) The instruction in raw bytes
};

class DisassemblerLines
{
public:
    DisassemblerLines() = default;

    bool coversAddress(quint64 address) const;
    void appendUnparsed(const QString &line);
    void appendLine(const DisassemblerLine &dl);
    void appendComment(const QString &line);
    // Mixed source/assembly: Retrieve contents of source (cached)
    void appendSourceLine(const QString &fileName, int line);
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
    int m_bytesLength = 0;
    QVector<DisassemblerLine> m_data;
    QHash<quint64, int> m_rowCache;
};

} // Debugger::Internal
