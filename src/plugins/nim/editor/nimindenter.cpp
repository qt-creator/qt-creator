/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimindenter.h"

#include "../tools/nimlexer.h"

#include <texteditor/icodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/tabsettings.h>

#include <QSet>
#include <QDebug>

namespace Nim {

NimIndenter::NimIndenter()
{}

bool NimIndenter::isElectricCharacter(const QChar &ch) const
{
    return NimIndenter::electricCharacters().contains(ch);
}

void NimIndenter::indentBlock(QTextDocument *document,
                              const QTextBlock &block,
                              const QChar &typedChar,
                              const TextEditor::TabSettings &settings)
{
    Q_UNUSED(document);
    Q_UNUSED(typedChar);

    const QString currentLine = block.text();

    const QTextBlock previousBlock = block.previous();
    const QString previousLine = previousBlock.text();
    const int previousState = previousBlock.userState();

    if (!previousBlock.isValid()) {
        settings.indentLine(block, 0);
        return;
    }

    // Calculate indentation
    int indentation = 0;
    if (rightTrimmed(currentLine).isEmpty()) {
        // Current line is empty so we calculate indentation based on previous line
        const int indentationDiff = calculateIndentationDiff(previousLine, previousState, settings.m_indentSize);
        indentation = settings.indentationColumn(previousLine) + indentationDiff;
    }
    else {
        // We don't change indentation if the line is already indented.
        // This is safer but sub optimal
        indentation = settings.indentationColumn(block.text());
    }

    // Sets indentation
    settings.indentLine(block, std::max(0, indentation));
}

const QSet<QChar> &NimIndenter::electricCharacters()
{
    static QSet<QChar> result{QLatin1Char(':'), QLatin1Char('=')};
    return result;
}

bool NimIndenter::startsBlock(const QString &line, int state) const
{
    NimLexer lexer(line.constData(), line.length(), static_cast<NimLexer::State>(state));

    // Read until end of line and save the last token
    NimLexer::Token previous;
    NimLexer::Token current = lexer.next();
    while (current.type != NimLexer::TokenType::EndOfText) {
        switch (current.type) {
        case NimLexer::TokenType::Comment:
        case NimLexer::TokenType::Documentation:
            break;
        default:
            previous = current;
            break;
        }
        current = lexer.next();
    }

    // electric characters start a new block, and are operators
    if (previous.type == NimLexer::TokenType::Operator) {
        QStringRef ref = line.midRef(previous.begin, previous.length);
        return ref.isEmpty() ? false : electricCharacters().contains(ref.at(0));
    }

    // some keywords starts a new block
    if (previous.type == NimLexer::TokenType::Keyword) {
        QStringRef ref = line.midRef(previous.begin, previous.length);
        return ref == QLatin1String("type")
               || ref == QLatin1String("var")
               || ref == QLatin1String("let")
               || ref == QLatin1String("enum")
               || ref == QLatin1String("object");
    }

    return false;
}

bool NimIndenter::endsBlock(const QString &line, int state) const
{
    NimLexer lexer(line.constData(), line.length(), static_cast<NimLexer::State>(state));

    // Read until end of line and save the last tokens
    NimLexer::Token previous;
    NimLexer::Token current = lexer.next();
    while (current.type != NimLexer::TokenType::EndOfText) {
        previous = current;
        current = lexer.next();
    }

    // Some keywords end a block
    if (previous.type == NimLexer::TokenType::Keyword) {
        QStringRef ref = line.midRef(previous.begin, previous.length);
        return ref == QLatin1String("return")
               || ref == QLatin1String("break")
               || ref == QLatin1String("continue");
    }

    return false;
}

int NimIndenter::calculateIndentationDiff(const QString &previousLine, int previousState, int indentSize) const
{
    if (previousLine.isEmpty())
        return 0;

    if (startsBlock(previousLine, previousState))
        return indentSize;

    if (endsBlock(previousLine, previousState))
        return -indentSize;

    return 0;
}

QString NimIndenter::rightTrimmed(const QString &str)
{
    int n = str.size() - 1;
    for (; n >= 0; --n) {
        if (!str.at(n).isSpace())
            return str.left(n + 1);
    }
    return QString();
}

}
