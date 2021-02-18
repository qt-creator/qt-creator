/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "diagnosticmanager.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textmark.h>
#include <texteditor/textstyles.h>
#include <utils/utilsicons.h>

#include <QTextEdit>

using namespace LanguageServerProtocol;
using namespace Utils;
using namespace TextEditor;

namespace LanguageClient {

class TextMark : public TextEditor::TextMark
{
public:
    TextMark(const FilePath &fileName, const Diagnostic &diag, const Id &clientId)
        : TextEditor::TextMark(fileName, diag.range().start().line() + 1, clientId)
        , m_diagnostic(diag)
    {
        setLineAnnotation(diag.message());
        setToolTip(diag.message());
        const bool isError
            = diag.severity().value_or(DiagnosticSeverity::Hint) == DiagnosticSeverity::Error;
        setColor(isError ? Theme::CodeModel_Error_TextMarkColor
                         : Theme::CodeModel_Warning_TextMarkColor);

        setIcon(isError ? Icons::CODEMODEL_ERROR.icon()
                        : Icons::CODEMODEL_WARNING.icon());
    }

    const Diagnostic &diagnostic() const { return m_diagnostic; }

private:
    const Diagnostic m_diagnostic;
};

DiagnosticManager::DiagnosticManager(const Id &clientId)
    : m_clientId(clientId)
{}

DiagnosticManager::~DiagnosticManager()
{
    clearDiagnostics();
}

void DiagnosticManager::setDiagnostics(const LanguageServerProtocol::DocumentUri &uri,
                                       const QList<LanguageServerProtocol::Diagnostic> &diagnostics,
                                       const Utils::optional<int> &version)
{
    removeDiagnostics(uri);
    m_diagnostics[uri] = {version, diagnostics};
}

void DiagnosticManager::hideDiagnostics(TextDocument *doc)
{
    if (!doc)
        return;

    for (BaseTextEditor *editor : BaseTextEditor::textEditorsForDocument(doc))
        editor->editorWidget()->setExtraSelections(TextEditorWidget::CodeWarningsSelection, {});
    qDeleteAll(Utils::filtered(doc->marks(), Utils::equal(&TextMark::category, m_clientId)));
}

void DiagnosticManager::removeDiagnostics(const LanguageServerProtocol::DocumentUri &uri)
{
    hideDiagnostics(TextDocument::textDocumentForFilePath(uri.toFilePath()));
    m_diagnostics.remove(uri);
}

static QTextEdit::ExtraSelection toDiagnosticsSelections(const Diagnostic &diagnostic,
                                                         QTextDocument *textDocument)
{
    QTextCursor cursor(textDocument);
    cursor.setPosition(diagnostic.range().start().toPositionInDocument(textDocument));
    cursor.setPosition(diagnostic.range().end().toPositionInDocument(textDocument),
                       QTextCursor::KeepAnchor);

    const FontSettings &fontSettings = TextEditorSettings::fontSettings();
    const DiagnosticSeverity severity = diagnostic.severity().value_or(DiagnosticSeverity::Warning);
    const TextStyle style = severity == DiagnosticSeverity::Error ? C_ERROR : C_WARNING;

    return QTextEdit::ExtraSelection{cursor, fontSettings.toTextCharFormat(style)};
}

void DiagnosticManager::showDiagnostics(const DocumentUri &uri)
{
    const FilePath &filePath = uri.toFilePath();
    if (TextDocument *doc = TextDocument::textDocumentForFilePath(filePath)) {
        QList<QTextEdit::ExtraSelection> extraSelections;
        const VersionedDiagnostics &versionedDiagnostics =  m_diagnostics.value(uri);
        const int docRevision = doc->document()->revision();
        if (versionedDiagnostics.version.value_or(docRevision) == docRevision) {
            for (const Diagnostic &diagnostic : versionedDiagnostics.diagnostics) {
                doc->addMark(new TextMark(filePath, diagnostic, m_clientId));
                extraSelections << toDiagnosticsSelections(diagnostic, doc->document());
            }
        }

        for (BaseTextEditor *editor : BaseTextEditor::textEditorsForDocument(doc)) {
            editor->editorWidget()->setExtraSelections(TextEditorWidget::CodeWarningsSelection,
                                                       extraSelections);
        }
    }
}

void DiagnosticManager::clearDiagnostics()
{
    for (const DocumentUri &uri : m_diagnostics.keys())
        removeDiagnostics(uri);
}

QList<Diagnostic> DiagnosticManager::diagnosticsAt(const DocumentUri &uri,
                                                   const QTextCursor &cursor) const
{
    const int documentRevision = cursor.document()->revision();
    auto it = m_diagnostics.find(uri);
    if (it == m_diagnostics.end())
        return {};
    if (documentRevision != it->version.value_or(documentRevision))
        return {};
    return Utils::filtered(it->diagnostics, [range = Range(cursor)](const Diagnostic &diagnostic) {
        return diagnostic.range().overlaps(range);
    });
}

} // namespace LanguageClient
