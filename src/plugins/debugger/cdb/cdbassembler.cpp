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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cdbassembler.h"
#include "cdbdebugoutput.h"
#include "cdbdebugengine_p.h"
#include "cdbsymbolgroupcontext.h"

#include "registerhandler.h"

#include <QtCore/QVector>

// Format a hex address with a given field width if possible. Convert
// to number to ensure it is not truncated should it be larger than the
// field width. Check the 64 bit address format '00000001`40002c84'
static inline void formatAddress(QTextStream &str, QString hexAddressS, int fieldWidth)
{
    if (hexAddressS.size() > 9) {
        const int sepPos = hexAddressS.size() - 9;
        if (hexAddressS.at(sepPos) == QLatin1Char('`'))
            hexAddressS.remove(sepPos, 1);
    }
    const QChar oldPadChar = str.padChar();
    const int oldFieldWidth = str.fieldWidth();
    const int oldIntegerBase = str.integerBase();
    str.setFieldWidth(fieldWidth);
    str.setPadChar(QLatin1Char('0'));
    str.setIntegerBase(16);
    str << hexAddressS.toULongLong(0, 16);
    str.setFieldWidth(oldFieldWidth);
    str.setPadChar(oldPadChar);
    str.setIntegerBase(oldIntegerBase);
}

namespace Debugger {
namespace Internal {

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
        reg.name = QString::fromUtf16(reinterpret_cast<const ushort *>(wszBuf)).toLatin1();
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

/* Parse a disassembler line:
 * \code
module!class::foo:
                        004017cf cc int 3
77 mainwindow.cpp       004018ff 8d4da8           lea     ecx,[ebp-0x58]
\endcode */

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
    // Format line. Start with address with the field width given,
    // which is important for setting the marker.
    const int addressToken = hasSourceFile ? 2 : 0;
    m_str << "0x";
    if (m_str.fieldWidth() == m_addressFieldWidth) {
        m_str << tokens.at(addressToken);
    } else {
        formatAddress(m_str, tokens.at(addressToken), m_addressFieldWidth);
    }
    m_str << ' ';
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
    for (int i = addressToken + 1; i < tokenCount; i++)
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

bool dissassemble(CIDebugClient *client,
                  CIDebugControl *ctl,
                  ULONG64 offset,
                  unsigned long beforeLines,
                  unsigned long afterLines,
                  int addressFieldWidth,
                  QTextStream &str,
                  QString *errorMessage)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << offset;

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
        *errorMessage= QString::fromLatin1("Unable to disassamble at 0x%1: %2").
                       arg(offset, 0, 16).arg(msgComFailed("OutputDisassemblyLines", hr));
        return false;
    }
    DisassemblerOutputParser parser(str, addressFieldWidth);
    parser.parse(stringHandler.result().split(QLatin1Char('\n')));
    return true;
}

} // namespace Internal
} // namespace Debugger
