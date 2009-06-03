/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdbassembler.h"
#include "cdbdebugoutput.h"
#include "cdbdebugengine_p.h"
#include "cdbsymbolgroupcontext.h"

#include "disassemblerhandler.h"
#include "registerhandler.h"

#include <QtCore/QVector>

namespace Debugger {
namespace Internal {

typedef QList<DisassemblerLine> DisassemblerLineList;

bool getRegisters(CIDebugControl *ctl,
                  CIDebugRegisters *ireg,
                  QList<Register> *registers,
                  QString *errorMessage, int base)
{
    registers->clear();
    ULONG count;
    HRESULT hr = ireg->GetNumberRegisters(&count);
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("GetNumberRegisters", hr);
        return false;
    }
    if (!count)
        return true;
    // Retrieve names
    WCHAR wszBuf[MAX_PATH];
    for (ULONG r = 0; r < count; r++) {
        hr = ireg->GetDescriptionWide(r, wszBuf, MAX_PATH - 1, 0, 0);
        if (FAILED(hr)) {
            *errorMessage= msgComFailed("GetDescriptionWide", hr);
            return false;
        }
        Register reg;
        reg.name = QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf));
        registers->push_back(reg);
    }
    // get values
    QVector<DEBUG_VALUE> values(count);
    DEBUG_VALUE *valuesPtr = &(*values.begin());
    memset(valuesPtr, 0, count * sizeof(DEBUG_VALUE));
    hr = ireg->GetValues(count, 0, 0, valuesPtr);
    if (FAILED(hr)) {
        *errorMessage= msgComFailed("GetValues", hr);
        return false;
    }
    if (base < 2)
        base = 10;
    for (ULONG r = 0; r < count; r++)
        (*registers)[r].value = CdbSymbolGroupContext::debugValueToString(values.at(r), ctl, 0, base);
    return true;
}

// Output parser for disassembler lines.
// It uses the source file lines as symbol until it encounters
// a C++ symbol (function entered), from which then on
// it uses that symbol.
class DisassemblerOutputParser
{
public:
    explicit DisassemblerOutputParser(DisassemblerLineList *list);

    void parse(const QStringList &l);

private:
    enum ParseResult { ParseOk, ParseIgnore, ParseFailed };
    ParseResult parseDisassembled(const QString &in, DisassemblerLine* l);

    DisassemblerLineList *m_list;
    QString m_sourceSymbol;
    int m_sourceSymbolOffset;
};

DisassemblerOutputParser::DisassemblerOutputParser(DisassemblerLineList *list) :
    m_list(list),
    m_sourceSymbolOffset(0)
{
}

// Parse a disassembler line:
//  module!class::foo:
//                          004017cf cc int 3
//  77 mainwindow.cpp       004018ff 8d4da8           lea     ecx,[ebp-0x58]
DisassemblerOutputParser::ParseResult
    DisassemblerOutputParser::parseDisassembled(const QString &in, DisassemblerLine* l)
{
    l->clear();

    // Check if there is a source file
    if (in.size() < 7)
        return ParseIgnore;
    const bool hasSourceFile = !in.at(6).isSpace();

    // Sometimes, empty lines occur
    const QString simplified = in.simplified();
    if (simplified.isEmpty())
        return ParseIgnore;

    QStringList tokens = simplified.split(QLatin1Char(' '), QString::SkipEmptyParts);
    // Check for symbols as 'module!class::foo:' (start of function encountered)
    if (tokens.size() == 1) {
        QString symbol = tokens.front();
        if (symbol.endsWith(QLatin1Char(':'))  && symbol.contains(QLatin1Char('!'))) {
            symbol.truncate(symbol.size() - 1);
            m_sourceSymbol = symbol;
            m_sourceSymbolOffset = 0;
        }
        return ParseIgnore;
    }
    if (tokens.size() < 2)
        return ParseIgnore;
    // Symbol display: Do we know a symbol?
    if (!m_sourceSymbol.isEmpty()) {
        l->symbol = QString(QLatin1Char('<'));
        l->symbol += m_sourceSymbol;
        if (m_sourceSymbolOffset) {
            l->symbol += QLatin1Char('+');
            l->symbol += QString::number(m_sourceSymbolOffset);
        }
        l->symbol += QLatin1Char('>');
        m_sourceSymbolOffset++;
    }
    // Read source file information: If we don't know a symbol yet,
    // use the source file.
    if (hasSourceFile) {
        if (l->symbol.isEmpty()) {
            l->symbol = tokens.at(1);
            l->symbol += QLatin1Char('+');
            l->symbol += tokens.front();
        }
        tokens.pop_front();
        tokens.pop_front();
    }
    l->symbolDisplay = l->symbol;
    // Get offset address and instruction
    if (tokens.size() < 3)
        return ParseFailed;
    l->addressDisplay = l->address = tokens.front();    
    tokens.pop_front();
    // The rest is effective address & instructions
    if (tokens.size() > 1)
        tokens.pop_front();
    l->mnemonic = tokens.join(QString(QLatin1Char(' ')));     
    return ParseOk;
}

void DisassemblerOutputParser::parse(const QStringList &l)
{
    DisassemblerLine dLine;
    foreach(const QString &line, l) {
        switch (parseDisassembled(line, &dLine)) {
        case ParseOk:
            m_list->push_back(dLine);
            break;
        case ParseIgnore:
            break;
        case ParseFailed:
            qWarning("Failed to parse '%s'\n", qPrintable(line));
            break;
        }
    }
}

bool dissassemble(CIDebugClient *client,
                  CIDebugControl *ctl,
                  ULONG64 offset,
                  unsigned long beforeLines,
                  unsigned long afterLines,
                  QList<DisassemblerLine> *lines,
                  QString *errorMessage)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << offset;
    lines->clear();
    const ULONG flags = DEBUG_DISASM_MATCHING_SYMBOLS|DEBUG_DISASM_SOURCE_LINE_NUMBER|DEBUG_DISASM_SOURCE_FILE_NAME;
    // Catch the output by temporarily setting another handler.
    // We use the method that outputs to the output handler as it
    // conveniently provides the 'beforeLines' context (stepping back
    // in assembler code). We build a complete string first as line breaks
    // may occur in-between messages.
    StringOutputHandler stringHandler;
    OutputRedirector redir(client, &stringHandler);
    // For some reason, we need to output to "all clients"
    const HRESULT hr =  ctl->OutputDisassemblyLines(DEBUG_OUTCTL_ALL_CLIENTS,
                                                    beforeLines, beforeLines + afterLines,
                                                    offset, flags, 0, 0, 0, 0);
    if (FAILED(hr)) {
        *errorMessage= QString::fromLatin1("Unable to dissamble at 0x%1: %2").
                       arg(QString::number(offset, 16), msgComFailed("OutputDisassemblyLines", hr));
        return false;
    }
    DisassemblerOutputParser parser(lines);
    parser.parse(stringHandler.result().split(QLatin1Char('\n')));
    return true;
}

} // namespace Internal
} // namespace Debugger
