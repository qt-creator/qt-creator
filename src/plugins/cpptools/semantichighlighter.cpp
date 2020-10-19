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

#include "semantichighlighter.h"

#include <texteditor/fontsettings.h>
#include <texteditor/semantichighlighter.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>

#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QTextDocument>

using namespace TextEditor;

using SemanticHighlighter::incrementalApplyExtraAdditionalFormats;
using SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd;

static Q_LOGGING_CATEGORY(log, "qtc.cpptools.semantichighlighter", QtWarningMsg)

namespace CppTools {

static const QList<std::pair<HighlightingResult, QTextBlock>>
splitRawStringLiteral(const HighlightingResult &result, const QTextBlock &startBlock)
{
    if (result.textStyles.mainStyle != C_STRING)
        return {{result, startBlock}};

    QTextCursor cursor(startBlock);
    cursor.setPosition(cursor.position() + result.column - 1);
    cursor.setPosition(cursor.position() + result.length, QTextCursor::KeepAnchor);
    const QString theString = cursor.selectedText();

    // Find all the components of a raw string literal. If we don't succeed, then it's
    // something else.
    if (!theString.endsWith('"'))
        return {{result, startBlock}};
    int rOffset = -1;
    if (theString.startsWith("R\"")) {
        rOffset = 0;
    } else if (theString.startsWith("LR\"")
               || theString.startsWith("uR\"")
               || theString.startsWith("UR\"")) {
        rOffset = 1;
    } else if (theString.startsWith("u8R\"")) {
        rOffset = 2;
    }
    if (rOffset == -1)
        return {{result, startBlock}};
    const int delimiterOffset = rOffset + 2;
    const int openParenOffset = theString.indexOf('(', delimiterOffset);
    if (openParenOffset == -1)
        return {{result, startBlock}};
    const QStringView delimiter = theString.mid(delimiterOffset, openParenOffset - delimiterOffset);
    const int endDelimiterOffset = theString.length() - 1 - delimiter.length();
    if (theString.mid(endDelimiterOffset, delimiter.length()) != delimiter)
        return {{result, startBlock}};
    if (theString.at(endDelimiterOffset - 1) != ')')
        return {{result, startBlock}};

    // Now split the result. For clarity, we display only the actual content as a string,
    // and the rest (including the delimiter) as a keyword.
    HighlightingResult prefix = result;
    prefix.textStyles.mainStyle = C_KEYWORD;
    prefix.textStyles.mixinStyles = {};
    prefix.length = delimiterOffset + delimiter.length() + 1;
    cursor.setPosition(startBlock.position() + result.column - 1 + prefix.length);
    QTextBlock stringBlock = cursor.block();
    HighlightingResult actualString = result;
    actualString.line = stringBlock.blockNumber() + 1;
    actualString.column = cursor.positionInBlock() + 1;
    actualString.length = endDelimiterOffset - openParenOffset - 2;
    cursor.setPosition(cursor.position() + actualString.length);
    QTextBlock suffixBlock = cursor.block();
    HighlightingResult suffix = result;
    suffix.textStyles.mainStyle = C_KEYWORD;
    suffix.textStyles.mixinStyles = {};
    suffix.line = suffixBlock.blockNumber() + 1;
    suffix.column = cursor.positionInBlock() + 1;
    suffix.length = delimiter.length() + 2;
    QTC_CHECK(prefix.length + actualString.length + suffix.length == result.length);

    return {{prefix, startBlock}, {actualString, stringBlock}, {suffix, suffixBlock}};
}

SemanticHighlighter::SemanticHighlighter(TextDocument *baseTextDocument)
    : QObject(baseTextDocument)
    , m_baseTextDocument(baseTextDocument)
{
    QTC_CHECK(m_baseTextDocument);
    updateFormatMapFromFontSettings();
}

SemanticHighlighter::~SemanticHighlighter()
{
    if (m_watcher) {
        disconnectWatcher();
        m_watcher->cancel();
        m_watcher->waitForFinished();
    }
}

void SemanticHighlighter::setHighlightingRunner(HighlightingRunner highlightingRunner)
{
    m_highlightingRunner = highlightingRunner;
}

void SemanticHighlighter::run()
{
    QTC_ASSERT(m_highlightingRunner, return);

    qCDebug(log) << "SemanticHighlighter: run()";

    if (m_watcher) {
        disconnectWatcher();
        m_watcher->cancel();
    }
    m_watcher.reset(new QFutureWatcher<HighlightingResult>);
    connectWatcher();

    m_revision = documentRevision();
    m_watcher->setFuture(m_highlightingRunner());
}

void SemanticHighlighter::onHighlighterResultAvailable(int from, int to)
{
    if (documentRevision() != m_revision)
        return; // outdated
    if (!m_watcher || m_watcher->isCanceled())
        return; // aborted

    qCDebug(log) << "onHighlighterResultAvailable()" << from << to;

    SyntaxHighlighter *highlighter = m_baseTextDocument->syntaxHighlighter();
    QTC_ASSERT(highlighter, return);
    incrementalApplyExtraAdditionalFormats(highlighter, m_watcher->future(), from, to, m_formatMap,
                                           &splitRawStringLiteral);
}

void SemanticHighlighter::onHighlighterFinished()
{
    QTC_ASSERT(m_watcher, return);
    if (!m_watcher->isCanceled() && documentRevision() == m_revision) {
        SyntaxHighlighter *highlighter = m_baseTextDocument->syntaxHighlighter();
        if (QTC_GUARD(highlighter)) {
            qCDebug(log) << "onHighlighterFinished() - clearing formats";
            clearExtraAdditionalFormatsUntilEnd(highlighter, m_watcher->future());
        }
    }
    m_watcher.reset();
}

void SemanticHighlighter::connectWatcher()
{
    using Watcher = QFutureWatcher<HighlightingResult>;
    connect(m_watcher.data(), &Watcher::resultsReadyAt,
            this, &SemanticHighlighter::onHighlighterResultAvailable);
    connect(m_watcher.data(), &Watcher::finished,
            this, &SemanticHighlighter::onHighlighterFinished);
}

void SemanticHighlighter::disconnectWatcher()
{
    using Watcher = QFutureWatcher<HighlightingResult>;
    disconnect(m_watcher.data(), &Watcher::resultsReadyAt,
               this, &SemanticHighlighter::onHighlighterResultAvailable);
    disconnect(m_watcher.data(), &Watcher::finished,
               this, &SemanticHighlighter::onHighlighterFinished);
}

unsigned SemanticHighlighter::documentRevision() const
{
    return m_baseTextDocument->document()->revision();
}

void SemanticHighlighter::updateFormatMapFromFontSettings()
{
    QTC_ASSERT(m_baseTextDocument, return);

    const FontSettings &fs = m_baseTextDocument->fontSettings();

    m_formatMap[TypeUse] = fs.toTextCharFormat(C_TYPE);
    m_formatMap[LocalUse] = fs.toTextCharFormat(C_LOCAL);
    m_formatMap[FieldUse] = fs.toTextCharFormat(C_FIELD);
    m_formatMap[EnumerationUse] = fs.toTextCharFormat(C_ENUMERATION);
    m_formatMap[VirtualMethodUse] = fs.toTextCharFormat(C_VIRTUAL_METHOD);
    m_formatMap[LabelUse] = fs.toTextCharFormat(C_LABEL);
    m_formatMap[MacroUse] = fs.toTextCharFormat(C_PREPROCESSOR);
    m_formatMap[FunctionUse] = fs.toTextCharFormat(C_FUNCTION);
    m_formatMap[FunctionDeclarationUse] =
            fs.toTextCharFormat(TextStyles::mixinStyle(C_FUNCTION, C_DECLARATION));
    m_formatMap[VirtualFunctionDeclarationUse] =
            fs.toTextCharFormat(TextStyles::mixinStyle(C_VIRTUAL_METHOD, C_DECLARATION));
    m_formatMap[PseudoKeywordUse] = fs.toTextCharFormat(C_KEYWORD);
    m_formatMap[StringUse] = fs.toTextCharFormat(C_STRING);
}

} // namespace CppTools
