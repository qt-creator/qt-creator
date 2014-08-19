/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "semantichighlighter.h"

#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>

#include <utils/qtcassert.h>

enum { debug = 0 };

using namespace CPlusPlus;
using TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats;
using TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd;

namespace CppTools {

SemanticHighlighter::SemanticHighlighter(TextEditor::BaseTextDocument *baseTextDocument)
    : QObject(baseTextDocument)
    , m_baseTextDocument(baseTextDocument)
    , m_revision(0)
{
    QTC_CHECK(m_baseTextDocument);

    connect(baseTextDocument, &TextEditor::BaseTextDocument::fontSettingsChanged,
            this, &SemanticHighlighter::onDocumentFontSettingsChanged);

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

    if (debug)
        qDebug() << "SemanticHighlighter: run()";

    if (m_watcher) {
        disconnectWatcher();
        m_watcher->cancel();
    }
    m_watcher.reset(new QFutureWatcher<TextEditor::HighlightingResult>);
    connectWatcher();

    m_revision = documentRevision();
    m_watcher->setFuture(m_highlightingRunner());
}

void SemanticHighlighter::onDocumentFontSettingsChanged()
{
    updateFormatMapFromFontSettings();
    run();
}

void SemanticHighlighter::onHighlighterResultAvailable(int from, int to)
{
    if (documentRevision() != m_revision)
        return; // outdated
    else if (!m_watcher || m_watcher->isCanceled())
        return; // aborted

    if (debug)
        qDebug() << "SemanticHighlighter: onHighlighterResultAvailable()" << from << to;

    TextEditor::SyntaxHighlighter *highlighter = m_baseTextDocument->syntaxHighlighter();
    QTC_ASSERT(highlighter, return);
    incrementalApplyExtraAdditionalFormats(highlighter, m_watcher->future(), from, to, m_formatMap);
}

void SemanticHighlighter::onHighlighterFinished()
{
    QTC_ASSERT(m_watcher, return);
    if (!m_watcher->isCanceled() && documentRevision() == m_revision) {
        TextEditor::SyntaxHighlighter *highlighter = m_baseTextDocument->syntaxHighlighter();
        QTC_CHECK(highlighter);
        if (highlighter) {
            if (debug)
                qDebug() << "SemanticHighlighter: onHighlighterFinished() - clearing formats";
            clearExtraAdditionalFormatsUntilEnd(highlighter, m_watcher->future());
        }
    }
    m_watcher.reset();
}

void SemanticHighlighter::connectWatcher()
{
    typedef QFutureWatcher<TextEditor::HighlightingResult> Watcher;
    connect(m_watcher.data(), &Watcher::resultsReadyAt,
            this, &SemanticHighlighter::onHighlighterResultAvailable);
    connect(m_watcher.data(), &Watcher::finished,
            this, &SemanticHighlighter::onHighlighterFinished);
}

void SemanticHighlighter::disconnectWatcher()
{
    typedef QFutureWatcher<TextEditor::HighlightingResult> Watcher;
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
    const TextEditor::FontSettings &fs = m_baseTextDocument->fontSettings();

    m_formatMap[TypeUse] = fs.toTextCharFormat(TextEditor::C_TYPE);
    m_formatMap[LocalUse] = fs.toTextCharFormat(TextEditor::C_LOCAL);
    m_formatMap[FieldUse] = fs.toTextCharFormat(TextEditor::C_FIELD);
    m_formatMap[EnumerationUse] = fs.toTextCharFormat(TextEditor::C_ENUMERATION);
    m_formatMap[VirtualMethodUse] = fs.toTextCharFormat(TextEditor::C_VIRTUAL_METHOD);
    m_formatMap[LabelUse] = fs.toTextCharFormat(TextEditor::C_LABEL);
    m_formatMap[MacroUse] = fs.toTextCharFormat(TextEditor::C_PREPROCESSOR);
    m_formatMap[FunctionUse] = fs.toTextCharFormat(TextEditor::C_FUNCTION);
    m_formatMap[PseudoKeywordUse] = fs.toTextCharFormat(TextEditor::C_KEYWORD);
    m_formatMap[StringUse] = fs.toTextCharFormat(TextEditor::C_STRING);
}

} // namespace CppTools
