/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "disassemblerlines.h"
#include "sourceutils.h"

#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QTextStream>

namespace Debugger {
namespace Internal {

void DisassemblerLine::fromString(const QString &unparsed)
{
    int pos = -1;
    for (int i = 0; i != unparsed.size(); ++i) {
        const uint c = unparsed.at(i).unicode();
        if (c == ' ' || c == ':' || c == '\t') {
            pos = i;
            break;
        }
    }

    // Mac gdb has an overflow reporting 64bit addresses causing the instruction
    // to follow the last digit "0x000000013fff4810mov 1,1". Truncate here.
    if (pos > 19 && unparsed.mid(3, 16).toULongLong())
        pos = 19;

    QString addr = unparsed.left(pos);
    // MSVC 64bit: Remove 64bit separator 00000000`00a45000'.
    if (addr.size() >= 9 && addr.at(8) == QLatin1Char('`'))
        addr.remove(8, 1);

    if (addr.endsWith(QLatin1Char(':'))) // clang
        addr.chop(1);
    if (addr.startsWith(QLatin1String("0x")))
        addr.remove(0, 2);
    bool ok = false;
    address = addr.toULongLong(&ok, 16);
    if (ok)
        data = unparsed.mid(pos + 1);
    else
        data = unparsed;
}

quint64 DisassemblerLines::startAddress() const
{
    for (int i = 0; i < m_data.size(); ++i)
        if (m_data.at(i).address)
            return m_data.at(i).address;
    return 0;
}

quint64 DisassemblerLines::endAddress() const
{
    for (int i = m_data.size()- 1; i >= 0; --i)
        if (m_data.at(i).address)
            return m_data.at(i).address;
    return 0;
}

int DisassemblerLines::lineForAddress(quint64 address) const
{
    return m_rowCache.value(address);
}

bool DisassemblerLines::coversAddress(quint64 address) const
{
    return m_rowCache.value(address) != 0;
}

void DisassemblerLines::appendLine(const DisassemblerLine &dl)
{
    m_data.append(dl);
    m_rowCache[dl.address] = m_data.size();
}

// Append source line: Cache current file.
struct SourceFileCache
{
    QString fileName;
    QStringList lines;
};

Q_GLOBAL_STATIC(SourceFileCache, sourceFileCache)

void DisassemblerLines::appendSourceLine(const QString &fileName, uint lineNumber)
{

    if (fileName.isEmpty() || lineNumber == 0)
        return;
    lineNumber--; // Fix 1..n range.
    SourceFileCache *cache = sourceFileCache();
    if (fileName != cache->fileName) {
        cache->fileName = fileName;
        cache->lines.clear();
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream ts(&file);
            cache->lines = ts.readAll().split(QLatin1Char('\n'));
        }
    }
    if (lineNumber >= uint(cache->lines.size()))
        return;
    DisassemblerLine dl;
    dl.lineNumber = lineNumber;
    dl.data = cache->lines.at(lineNumber);
    appendLine(dl);
}

void DisassemblerLines::appendComment(const QString &line)
{
    DisassemblerLine dl;
    dl.data = line;
    appendLine(dl);
}

void DisassemblerLines::appendUnparsed(const QString &unparsed)
{
    QString line = unparsed.trimmed();
    if (line.isEmpty())
        return;
    if (line.startsWith(QLatin1String("Current language:")))
        return;
    if (line.startsWith(QLatin1String("Dump of assembler"))) {
        m_lastFunction.clear();
        return;
    }
    if (line.startsWith(QLatin1String("The current source")))
        return;
    if (line.startsWith(QLatin1String("End of assembler"))) {
        m_lastFunction.clear();
        return;
    }
    if (line.startsWith(QLatin1String("=> ")))
        line = line.mid(3);
    if (line.startsWith(QLatin1String("0x"))) {
        // Address line. Split at the tab.
        int tab1 = line.indexOf(QLatin1Char('\t'));
        if (tab1 == -1) {
            appendComment(line);
            return;
        }
        int tab2 = line.indexOf(QLatin1Char('\t'), tab1 + 1);
        if (tab2 == -1)
            tab2 = tab1;
        QString address = line.left(tab1);
        if (address.endsWith(QLatin1Char(':')))
            address.chop(1);
        int pos1 = address.indexOf(QLatin1Char('<')) + 1;
        DisassemblerLine dl;
        dl.bytes = line.mid(tab1, tab2 - tab1).trimmed();
        m_bytesLength = qMax(m_bytesLength, tab2 - tab1);
        dl.data = line.mid(tab2).trimmed();
        if (pos1 && address.indexOf(QLatin1String("<UNDEFINED> instruction:")) == -1) {
            if (address.endsWith(QLatin1Char('>')))
                address.chop(1);
            int pos2 = address.indexOf(QLatin1Char('+'), pos1);
            if (pos1 < pos2) {
                QString function = address.mid(pos1, pos2 - pos1);
                if (function != m_lastFunction) {
                    DisassemblerLine dl;
                    dl.data = "Function: " + function;
                    m_data.append(dl);
                    m_lastFunction = function;
                }
            }
            dl.address = address.left(pos1 - 1).toULongLong(0, 0);
            dl.function = m_lastFunction;
            dl.offset = address.mid(pos2).toUInt();
        } else {
            // Plain data like "0x0000cd64:\tadd\tlr, pc, lr\n"
            dl.address = address.toULongLong(0, 0);
            dl.function = m_lastFunction;
            dl.offset = 0;
        }
        m_rowCache[dl.address] = m_data.size() + 1;
        m_data.append(dl);
    } else {
        // Comment or code line.
        QTextStream ts(&line);
        DisassemblerLine dl;
        ts >> dl.lineNumber;
        dl.data = line.mid(ts.pos());
        m_data.append(dl);
    }
}

QString DisassemblerLine::toString(int maxOp) const
{
    const QString someSpace = "        ";
    QString str;
    if (isAssembler()) {
        if (address)
            str += QString("0x%1  ").arg(address, 0, 16);
        if (offset)
            str += QString("<+0x%2> ").arg(offset, 4, 16, QLatin1Char('0'));
        else
            str += "          ";
        str += QString("       %1 ").arg(bytes);
        str += QString(maxOp - bytes.size(), QLatin1Char(' '));
        str += data;
    } else if (isCode()) {
        str += someSpace;
        str += QString::number(lineNumber);
        if (hunk)
            str += QString(" [%1]").arg(hunk);
        else
            str += "    ";
        str += data;
    } else {
        str += someSpace;
        str += data;
    }
    return str;
}

QString DisassemblerLines::toString() const
{
    QString str;
    for (int i = 0, n = size(); i != n; ++i) {
        str += m_data.at(i).toString(m_bytesLength);
        str += QLatin1Char('\n');
    }
    return str;
}

} // namespace Internal
} // namespace Debugger
