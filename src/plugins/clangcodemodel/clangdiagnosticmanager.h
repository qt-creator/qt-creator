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

#pragma once

#include "clangtextmark.h"

#include <texteditor/refactoroverlay.h>

#include <clangsupport/diagnosticcontainer.h>

#include <QList>
#include <QSet>
#include <QTextEdit>
#include <QTimer>
#include <QVector>

#include <vector>

namespace TextEditor { class TextDocument; }

namespace ClangCodeModel {
namespace Internal {

class ClangDiagnosticManager
{
public:
    ClangDiagnosticManager(TextEditor::TextDocument *textDocument);
    ~ClangDiagnosticManager();

    void processNewDiagnostics(const QVector<ClangBackEnd::DiagnosticContainer> &allDiagnostics,
                               bool showTextMarkAnnotations);

    const QVector<ClangBackEnd::DiagnosticContainer> &diagnosticsWithFixIts() const;
    QList<QTextEdit::ExtraSelection> takeExtraSelections();
    TextEditor::RefactorMarkers takeFixItAvailableMarkers();

    bool hasDiagnosticsAt(uint line, uint column) const;
    QVector<ClangBackEnd::DiagnosticContainer> diagnosticsAt(uint line, uint column) const;

    void invalidateDiagnostics();
    void clearDiagnosticsWithFixIts();

private:
    void cleanMarks();
    QString filePath() const;
    void filterDiagnostics(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics);
    void generateEditorSelections();
    void generateTextMarks();
    void generateFixItAvailableMarkers();
    void addClangTextMarks(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics);
    void addFixItAvailableMarker(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                                 QSet<int> &lineNumbersWithFixItMarker);

private:
    TextEditor::TextDocument *m_textDocument;

    QVector<ClangBackEnd::DiagnosticContainer> m_warningDiagnostics;
    QVector<ClangBackEnd::DiagnosticContainer> m_errorDiagnostics;
    QVector<ClangBackEnd::DiagnosticContainer> m_fixItdiagnostics;
    QList<QTextEdit::ExtraSelection> m_extraSelections;
    TextEditor::RefactorMarkers m_fixItAvailableMarkers;
    std::vector<ClangTextMark *> m_clangTextMarks;
    bool m_firstDiagnostics = true;
    bool m_diagnosticsInvalidated = false;
    bool m_showTextMarkAnnotations = false;
    QTimer m_textMarkDelay;
};

} // namespace Internal
} // namespace ClangCodeModel
