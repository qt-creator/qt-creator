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

#include "disassemblerlines.h"
#include "debuggerstringutils.h"

#include <QDebug>
#include <QRegExp>
#include <QFile>
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

quint64 DisassemblerLine::addressFromDisassemblyLine(const QString &line)
{
    DisassemblerLine l;
    l.fromString(line);
    return l.address;
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
        // Address line.
        int pos1 = line.indexOf(QLatin1Char('<')) + 1;
        int posc = line.indexOf(QLatin1Char(':'));
        DisassemblerLine dl;
        if (pos1 && line.indexOf(QLatin1String("<UNDEFINED> instruction:")) == -1) {
            int pos2 = line.indexOf(QLatin1Char('+'), pos1);
            int pos3 = line.indexOf(QLatin1Char('>'), pos1);
            if (pos1 < pos2 && pos2 < pos3) {
                QString function = line.mid(pos1, pos2 - pos1);
                if (function != m_lastFunction) {
                    DisassemblerLine dl;
                    dl.data = _("Function: ") + function;
                    m_data.append(dl);
                    m_lastFunction = function;
                }
            }
            dl.address = line.left(pos1 - 1).toULongLong(0, 0);
            dl.function = m_lastFunction;
            dl.offset = line.mid(pos2, pos3 - pos2).toUInt();
            dl.data = line.mid(pos3 + 3).trimmed();
        } else {
            // Plain data like "0x0000cd64:\tadd\tlr, pc, lr\n"
            dl.address = line.left(posc).toULongLong(0, 0);
            dl.function = m_lastFunction;
            dl.offset = 0;
            dl.data = line.mid(posc + 1).trimmed();
        }
        m_rowCache[dl.address] = m_data.size() + 1;
        m_data.append(dl);
    } else {
        // Comment line.
        DisassemblerLine dl;
        dl.data = line;
        m_data.append(dl);
    }
}

QString DisassemblerLine::toString() const
{
    const QString someSpace = _("        ");
    QString str;
    if (isAssembler()) {
        if (address)
            str += _("0x%1  <+0x%2> ").arg(address, 0, 16)
                .arg(offset, 4, 16, QLatin1Char('0'));
        str += _("        ");
        str += data;
    } else if (isCode()) {
        str += someSpace;
        str += data;
    } else {
        str += someSpace;
        str += data;
    }
    return str;
}

} // namespace Internal
} // namespace Debugger
