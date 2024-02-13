// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsindenter.h"

#include <qmljstools/qmljsqtstylecodeformatter.h>
#include <texteditor/tabsettings.h>

#include <QTextDocument>
#include <QTextBlock>

using namespace TextEditor;

namespace QmlJSEditor {
namespace Internal {

class QmlJsIndenter final : public TextEditor::TextIndenter
{
public:
    explicit QmlJsIndenter(QTextDocument *doc)
        : TextEditor::TextIndenter(doc)
    {}

    bool isElectricCharacter(const QChar &ch) const final;
    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) final;
    void invalidateCache() final;

    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) final;
    int visualIndentFor(const QTextBlock &block,
                        const TextEditor::TabSettings &tabSettings) final;
    TextEditor::IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                                         const TextEditor::TabSettings &tabSettings,
                                                         int cursorPositionInEditor = -1) final;
};

bool QmlJsIndenter::isElectricCharacter(const QChar &ch) const
{
    return ch == QLatin1Char('{')
        || ch == QLatin1Char('}')
        || ch == QLatin1Char(']')
        || ch == QLatin1Char(':');
}

void QmlJsIndenter::indentBlock(const QTextBlock &block,
                           const QChar &typedChar,
                           const TextEditor::TabSettings &tabSettings,
                           int /*cursorPositionInEditor*/)
{
    const int depth = indentFor(block, tabSettings);
    if (depth == -1)
        return;

    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);
    codeFormatter.updateStateUntil(block);

    if (isElectricCharacter(typedChar)) {
        // only reindent the current line when typing electric characters if the
        // indent is the same it would be if the line were empty
        const int newlineIndent = codeFormatter.indentForNewLineAfter(block.previous());
        if (tabSettings.indentationColumn(block.text()) != newlineIndent)
            return;
    }

    tabSettings.indentLine(block, depth);
}

void QmlJsIndenter::invalidateCache()
{
    QmlJSTools::CreatorCodeFormatter codeFormatter;
    codeFormatter.invalidateCache(m_doc);
}

int QmlJsIndenter::indentFor(const QTextBlock &block,
                        const TextEditor::TabSettings &tabSettings,
                        int /*cursorPositionInEditor*/)
{
    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);
    codeFormatter.updateStateUntil(block);
    return codeFormatter.indentFor(block);
}

int QmlJsIndenter::visualIndentFor(const QTextBlock &block, const TextEditor::TabSettings &tabSettings)
{
    return indentFor(block, tabSettings);
}

TextEditor::IndentationForBlock QmlJsIndenter::indentationForBlocks(
    const QVector<QTextBlock> &blocks,
    const TextEditor::TabSettings &tabSettings,
    int /*cursorPositionInEditor*/)
{
    QmlJSTools::CreatorCodeFormatter codeFormatter(tabSettings);

    codeFormatter.updateStateUntil(blocks.last());

    TextEditor::IndentationForBlock ret;
    for (QTextBlock block : blocks)
        ret.insert(block.blockNumber(), codeFormatter.indentFor(block));
    return ret;
}

} // Internal

TextEditor::TextIndenter *createQmlJsIndenter(QTextDocument *doc)
{
    return new Internal::QmlJsIndenter(doc);
}

void indentQmlJs(QTextDocument *doc, int startLine, int endLine, const TextEditor::TabSettings &tabSettings)
{
    if (startLine <= 0)
        return;

    QTextCursor tc(doc);

    tc.beginEditBlock();
    for (int i = startLine; i <= endLine; i++) {
        // FIXME: block.next() should be faster.
        QTextBlock block = doc->findBlockByNumber(i);
        if (block.isValid()) {
            Internal::QmlJsIndenter indenter(doc);
            indenter.indentBlock(block, QChar::Null, tabSettings);
        }
    }
    tc.endEditBlock();
}

} // QmlJsEditor
