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

#include "clangdiagnosticfilter.h"
#include "clangdiagnosticmanager.h"

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QTextBlock>

namespace  {

QTextEdit::ExtraSelection createExtraSelections(const QTextCharFormat &mainformat,
                                                const QTextCursor &cursor,
                                                const QString &diagnosticText)
{
    QTextEdit::ExtraSelection extraSelection;

    extraSelection.format = mainformat;
    extraSelection.cursor = cursor;
    extraSelection.format.setToolTip(diagnosticText);

    return extraSelection;
}

int positionInText(QTextDocument *textDocument,
                   const ClangBackEnd::SourceLocationContainer &sourceLocationContainer)
{
    auto textBlock = textDocument->findBlockByNumber(int(sourceLocationContainer.line()) - 1);

    return textBlock.position() + int(sourceLocationContainer.column()) - 1;
}

void addRangeSelections(const ClangBackEnd::DiagnosticContainer &diagnostic,
                        QTextDocument *textDocument,
                        const QTextCharFormat &contextFormat,
                        const QString &diagnosticText,
                        QList<QTextEdit::ExtraSelection> &extraSelections)
{
    for (auto &&range : diagnostic.ranges()) {
        QTextCursor cursor(textDocument);
        cursor.setPosition(positionInText(textDocument, range.start()));
        cursor.setPosition(positionInText(textDocument, range.end()), QTextCursor::KeepAnchor);

        auto extraSelection = createExtraSelections(contextFormat, cursor, diagnosticText);

        extraSelections.push_back(std::move(extraSelection));
    }
}

QTextCursor createSelectionCursor(QTextDocument *textDocument,
                                  const ClangBackEnd::SourceLocationContainer &sourceLocationContainer)
{
    QTextCursor cursor(textDocument);
    cursor.setPosition(positionInText(textDocument, sourceLocationContainer));
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    if (!cursor.hasSelection()) {
        cursor.setPosition(positionInText(textDocument, sourceLocationContainer) - 1);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
    }

    return cursor;
}

bool isHelpfulChildDiagnostic(const ClangBackEnd::DiagnosticContainer &parentDiagnostic,
                              const ClangBackEnd::DiagnosticContainer &childDiagnostic)
{
    auto parentLocation = parentDiagnostic.location();
    auto childLocation = childDiagnostic.location();

    return parentLocation == childLocation;
}

QString diagnosticText(const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    QString text = diagnostic.category().toString()
            + QStringLiteral("\n\n")
            + diagnostic.text().toString();

#ifdef QT_DEBUG
    if (!diagnostic.disableOption().isEmpty()) {
        text += QStringLiteral(" (disable with ")
                + diagnostic.disableOption().toString()
                + QStringLiteral(")");
    }
#endif

    for (auto &&childDiagnostic : diagnostic.children()) {
        if (isHelpfulChildDiagnostic(diagnostic, childDiagnostic))
            text += QStringLiteral("\n  ") + childDiagnostic.text().toString();
    }

    return text;
}

void addSelections(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                   QTextDocument *textDocument,
                   const QTextCharFormat &mainFormat,
                   const QTextCharFormat &contextFormat,
                   QList<QTextEdit::ExtraSelection> &extraSelections)
{
    for (auto &&diagnostic : diagnostics) {
        auto cursor = createSelectionCursor(textDocument, diagnostic.location());

        auto text = diagnosticText(diagnostic);
        auto extraSelection = createExtraSelections(mainFormat, cursor, text);

        addRangeSelections(diagnostic, textDocument, contextFormat, text, extraSelections);

        extraSelections.push_back(std::move(extraSelection));
    }
}

void addWarningSelections(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                          QTextDocument *textDocument,
                          QList<QTextEdit::ExtraSelection> &extraSelections)
{
    const auto fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

    QTextCharFormat warningFormat = fontSettings.toTextCharFormat(TextEditor::C_WARNING);

    QTextCharFormat warningContextFormat = fontSettings.toTextCharFormat(TextEditor::C_WARNING_CONTEXT);

    addSelections(diagnostics, textDocument, warningFormat, warningContextFormat, extraSelections);
}

void addErrorSelections(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                        QTextDocument *textDocument,
                        QList<QTextEdit::ExtraSelection> &extraSelections)
{
    const auto fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

    QTextCharFormat errorFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR);
    QTextCharFormat errorContextFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR_CONTEXT);

    addSelections(diagnostics, textDocument, errorFormat, errorContextFormat, extraSelections);
}

} // anonymous

namespace ClangCodeModel {
namespace Internal {

ClangDiagnosticManager::ClangDiagnosticManager(TextEditor::TextDocument *textDocument)
    : m_textDocument(textDocument)
{
}

void ClangDiagnosticManager::generateTextMarks()
{
    m_clangTextMarks.clear();
    m_clangTextMarks.reserve(m_warningDiagnostics.size() + m_errorDiagnostics.size());

    addClangTextMarks(m_warningDiagnostics);
    addClangTextMarks(m_errorDiagnostics);
}

QList<QTextEdit::ExtraSelection> ClangDiagnosticManager::takeExtraSelections()
{
    auto extraSelections = m_extraSelections;

    m_extraSelections.clear();

    return extraSelections;
}

void ClangDiagnosticManager::clearDiagnosticsWithFixIts()
{
    m_fixItdiagnostics.clear();
}

void ClangDiagnosticManager::generateEditorSelections()
{
    m_extraSelections.clear();
    m_extraSelections.reserve(int(m_warningDiagnostics.size() + m_errorDiagnostics.size()));

    addWarningSelections(m_warningDiagnostics, m_textDocument->document(), m_extraSelections);
    addErrorSelections(m_errorDiagnostics, m_textDocument->document(), m_extraSelections);
}

void ClangDiagnosticManager::processNewDiagnostics(
        const QVector<ClangBackEnd::DiagnosticContainer> &allDiagnostics)
{
    filterDiagnostics(allDiagnostics);

    generateTextMarks();
    generateEditorSelections();

    clearWarningsAndErrors();
}

const QVector<ClangBackEnd::DiagnosticContainer> &
ClangDiagnosticManager::diagnosticsWithFixIts() const
{
    return m_fixItdiagnostics;
}

void ClangDiagnosticManager::addClangTextMarks(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    QTC_ASSERT(m_clangTextMarks.size() + diagnostics.size() <= m_clangTextMarks.capacity(), return);

    for (auto &&diagnostic : diagnostics) {
        m_clangTextMarks.emplace_back(filePath(),
                                      diagnostic.location().line(),
                                      diagnostic.severity());

        ClangTextMark &textMark = m_clangTextMarks.back();

        textMark.setBaseTextDocument(m_textDocument);

        m_textDocument->addMark(&textMark);
    }
}

void ClangDiagnosticManager::clearWarningsAndErrors()
{
    m_warningDiagnostics.clear();
    m_errorDiagnostics.clear();
}

QString ClangDiagnosticManager::filePath() const
{
    return m_textDocument->filePath().toString();
}

void ClangDiagnosticManager::filterDiagnostics(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    ClangDiagnosticFilter filter(filePath());
    filter.filter(diagnostics);

    m_warningDiagnostics = filter.takeWarnings();
    m_errorDiagnostics = filter.takeErrors();
    m_fixItdiagnostics = filter.takeFixIts();
}

} // namespace Internal
} // namespace ClangCodeModel
