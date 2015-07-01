/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "semantichighlighter.h"
#include "cppsemanticinfo.h"

#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>

#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QTextDocument>

using namespace CPlusPlus;
using TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats;
using TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd;

static Q_LOGGING_CATEGORY(log, "qtc.cpptools.semantichighlighter")

namespace CppTools {

SemanticHighlighter::SemanticHighlighter(TextEditor::TextDocument *baseTextDocument)
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
    m_watcher.reset(new QFutureWatcher<TextEditor::HighlightingResult>);
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
            qCDebug(log) << "onHighlighterFinished() - clearing formats";
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
    QTC_ASSERT(m_baseTextDocument, return);

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
