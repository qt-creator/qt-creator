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

#include "cdbassembler.h"
#include "cdbdebugoutput.h"
#include "cdbengine_p.h"
#include "cdbsymbolgroupcontext.h"

#include "registerhandler.h"

#include <QtCore/QVector>

namespace Debugger {
namespace Internal {

Registers getRegisters(CIDebugControl *ctl,
                       CIDebugRegisters *ireg,
                       QString *errorMessage, int base)
{

    ULONG count;
    HRESULT hr = ireg->GetNumberRegisters(&count);
    if (FAILED(hr)) {
        *errorMessage= CdbCore::msgComFailed("GetNumberRegisters", hr);
        return Registers();
    }
    if (!count)
        return Registers();
    Registers registers(count);
    // Retrieve names
    char buf[MAX_PATH];
    for (ULONG r = 0; r < count; r++) {
        hr = ireg->GetDescription(r, buf, MAX_PATH - 1, 0, 0);
        if (FAILED(hr)) {
            *errorMessage= CdbCore::msgComFailed("GetDescription", hr);
            return Registers();
        }
        registers[r].name = QByteArray(buf);
    }
    // get values
    QVector<DEBUG_VALUE> values(count);
    DEBUG_VALUE *valuesPtr = &(*values.begin());
    memset(valuesPtr, 0, count * sizeof(DEBUG_VALUE));
    hr = ireg->GetValues(count, 0, 0, valuesPtr);
    if (FAILED(hr)) {
        *errorMessage= CdbCore::msgComFailed("GetValues", hr);
        return Registers();
    }
    if (base < 2)
        base = 10;
    for (ULONG r = 0; r < count; r++)
        registers[r].value = CdbCore::debugValueToString(values.at(r), 0, base, ctl);
    return registers;
}

/* Output parser for disassembler lines: Parse a disassembler line:
 * \code
module!class::foo:
                        004017cf cc int 3
77 mainwindow.cpp       004018ff 8d4da8           lea     ecx,[ebp-0x58]
\endcode
 * and reformat to something like:
 * \code
00000001400043c9 mainwindow.cpp+296 90 nop
00000001400043ca mainwindow.cpp+296 488d8c24d8020000 lea rcx,[rsp+2D8h]
00000001400043d2 mainwindow.cpp+296 ff1500640300 call qword ptr [gitgui!_imp_??1QStringQEAAXZ (00000001`4003a7d8)]
\endcode
 * Reformatting brings address to the front for disassembler agent's extracting
 * the address for location marker to work.
 * Moves symbol information to the 2nd column, using the source file lines as
 * symbol until it encounters a C++ symbol (function entered), from which then on
 * it uses that symbol, indicating the offset.
 */

class DisassemblerOutputParser
{
    Q_DISABLE_COPY(DisassemblerOutputParser)
public:
    explicit DisassemblerOutputParser(QTextStream &str, int addressFieldWidth = 0);

    void parse(const QStringList &l);

private:
    enum ParseResult { ParseOk, ParseIgnore, ParseFailed };
    ParseResult parseDisassembled(const QString &in);

    const int m_addressFieldWidth;
    QTextStream &m_str;
    QString m_sourceSymbol;
    int m_sourceSymbolOffset;
};

DisassemblerOutputParser::DisassemblerOutputParser(QTextStream &str, int addressFieldWidth) :
    m_addressFieldWidth(addressFieldWidth),
    m_str(str),
    m_sourceSymbolOffset(0)
{
}

DisassemblerOutputParser::ParseResult
    DisassemblerOutputParser::parseDisassembled(const QString &in)
{
    // Check if there is a source file
    if (in.size() < 7)
        return ParseIgnore;
    const bool hasSourceFile = !in.at(6).isSpace();

    // Sometimes, empty lines occur
    const QString simplified = in.simplified();
    if (simplified.isEmpty())
        return ParseIgnore;

    const QStringList tokens = simplified.split(QLatin1Char(' '), QString::SkipEmptyParts);
    const int tokenCount = tokens.size();
    // Check for symbols as 'module!class::foo:' (start of function encountered)
    // and store as state.
    if (tokenCount == 1) {
        QString symbol = tokens.front();
        if (symbol.endsWith(QLatin1Char(':'))  && symbol.contains(QLatin1Char('!'))) {
            symbol.truncate(symbol.size() - 1);
            m_sourceSymbol = symbol;
            m_sourceSymbolOffset = 0;
        }
        return ParseIgnore;
    }
    if (tokenCount < 2)
        return ParseIgnore;
    if (tokenCount < 3)
        return ParseFailed;
    // Format line. Start with address which is important for setting the marker.
    // Fix CDB word separator for location marker hex conversion to work.
    const int addressTokenPos = hasSourceFile ? 2 : 0;
    QString addressToken = tokens.at(addressTokenPos);
    if (addressToken.size() > 9 && addressToken.at(8) == QLatin1Char('`'))
        addressToken.remove(8, 1);
    m_str << addressToken << ' ';
    // Symbol display: Do we know a symbol? -> Display with offset.
    // Else default to source file information.
    if (m_sourceSymbol.isEmpty()) {
        if (hasSourceFile)
            m_str << tokens.at(1) << '+' << tokens.front();
    } else {
        m_str << '<' << m_sourceSymbol;
        if (m_sourceSymbolOffset)
            m_str << '+' << m_sourceSymbolOffset;
        m_str << '>';
        m_sourceSymbolOffset++;
    }
    for (int i = addressTokenPos + 1; i < tokenCount; i++)
        m_str << ' ' << tokens.at(i);
    m_str << '\n';
    return ParseOk;
}

void DisassemblerOutputParser::parse(const QStringList &l)
{
    foreach(const QString &line, l) {
        switch (parseDisassembled(line)) {
        case ParseOk:
        case ParseIgnore:
            break;
        case ParseFailed:
            qWarning("Failed to parse '%s'\n", qPrintable(line));
            break;
        }
    }
}

bool dissassemble(CdbCore::CoreEngine *engine,
                  ULONG64 offset,
                  unsigned long beforeLines,
                  unsigned long afterLines,
                  QTextStream &str,
                  QString *errorMessage)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << offset;
    QString lines;
    if (!engine->dissassemble(offset, beforeLines, afterLines, &lines, errorMessage))
        return false;
    DisassemblerOutputParser parser(str);
    parser.parse(lines.split(QLatin1Char('\n')));
    return true;
}

} // namespace Internal
} // namespace Debugger
