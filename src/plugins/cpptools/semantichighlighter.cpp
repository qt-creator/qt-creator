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

static Q_LOGGING_CATEGORY(log, "qtc.cpptools.semantichighlighter")

namespace CppTools {

SemanticHighlighter::SemanticHighlighter(TextDocument *baseTextDocument)
    : QObject(baseTextDocument)
    , m_baseTextDocument(baseTextDocument)
    , m_revision(0)
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
    else if (!m_watcher || m_watcher->isCanceled())
        return; // aborted

    qCDebug(log) << "onHighlighterResultAvailable()" << from << to;

    SyntaxHighlighter *highlighter = m_baseTextDocument->syntaxHighlighter();
    QTC_ASSERT(highlighter, return);
    incrementalApplyExtraAdditionalFormats(highlighter, m_watcher->future(), from, to, m_formatMap);
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
    typedef QFutureWatcher<HighlightingResult> Watcher;
    connect(m_watcher.data(), &Watcher::resultsReadyAt,
            this, &SemanticHighlighter::onHighlighterResultAvailable);
    connect(m_watcher.data(), &Watcher::finished,
            this, &SemanticHighlighter::onHighlighterFinished);
}

void SemanticHighlighter::disconnectWatcher()
{
    typedef QFutureWatcher<HighlightingResult> Watcher;
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
