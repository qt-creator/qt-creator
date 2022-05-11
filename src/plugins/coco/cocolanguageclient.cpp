/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "cocolanguageclient.h"

#include <app/app_version.h>
#include <coreplugin/editormanager/editormanager.h>
#include <languageclient/languageclientinterface.h>
#include <projectexplorer/project.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textmark.h>
#include <utils/utilsicons.h>

#include <QTextEdit>

using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace Utils;
using namespace Core;

namespace Coco {

CocoLanguageClient::CocoLanguageClient(const FilePath &coco, const FilePath &csmes)
    : Client(clientInterface(coco, csmes))
{
    setName("Coco");
    setActivateDocumentAutomatically(false);
    LanguageFilter allFiles;
    allFiles.filePattern = QStringList{"*"};
    setSupportedLanguage(allFiles);
    connect(EditorManager::instance(),
            &EditorManager::documentOpened,
            this,
            &CocoLanguageClient::handleDocumentOpened);
    connect(EditorManager::instance(),
            &EditorManager::documentClosed,
            this,
            &CocoLanguageClient::handleDocumentClosed);
    connect(EditorManager::instance(),
            &EditorManager::editorOpened,
            this,
            &CocoLanguageClient::handleEditorOpened);
    for (IDocument *openDocument : DocumentModel::openedDocuments())
        handleDocumentOpened(openDocument);

    ClientInfo info;
    info.setName("CocoQtCreator");
    info.setVersion(Core::Constants::IDE_VERSION_DISPLAY);
    setClientInfo(info);

    initClientCapabilities();
}

CocoLanguageClient::~CocoLanguageClient()
{
    const QList<Core::IEditor *> &editors = Core::DocumentModel::editorsForOpenedDocuments();
    for (Core::IEditor *editor : editors) {
        if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor))
            textEditor->editorWidget()->removeHoverHandler(hoverHandler());
    }
}

BaseClientInterface *CocoLanguageClient::clientInterface(const FilePath &coco,
                                                         const FilePath &csmes)
{
    auto interface = new StdIOClientInterface();
    interface->setCommandLine(CommandLine(coco, {"--lsp-stdio", csmes.toUserOutput()}));
    return interface;
}

enum class CocoDiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4,

    CodeAdded = 100,
    PartiallyCovered = 101,
    NotCovered = 102,
    FullyCovered = 103,
    ManuallyValidated = 104,
    DeadCode = 105,
    ExecutionCountTooLow = 106,
    NotCoveredInfo = 107,
    CoveredInfo = 108,
    ManuallyValidatedInfo = 109
};

static TextEditor::TextStyle styleForSeverity(const CocoDiagnosticSeverity &severity)
{
    using namespace TextEditor;
    switch (severity) {
    case CocoDiagnosticSeverity::Error: return C_ERROR;
    case CocoDiagnosticSeverity::Warning: return C_WARNING;
    case CocoDiagnosticSeverity::Information: return C_WARNING;
    case CocoDiagnosticSeverity::Hint: return C_WARNING;
    case CocoDiagnosticSeverity::CodeAdded: return C_COCO_CODE_ADDED;
    case CocoDiagnosticSeverity::PartiallyCovered: return C_COCO_PARTIALLY_COVERED;
    case CocoDiagnosticSeverity::NotCovered: return C_COCO_NOT_COVERED;
    case CocoDiagnosticSeverity::FullyCovered: return C_COCO_FULLY_COVERED;
    case CocoDiagnosticSeverity::ManuallyValidated: return C_COCO_MANUALLY_VALIDATED;
    case CocoDiagnosticSeverity::DeadCode: return C_COCO_DEAD_CODE;
    case CocoDiagnosticSeverity::ExecutionCountTooLow: return C_COCO_EXECUTION_COUNT_TOO_LOW;
    case CocoDiagnosticSeverity::NotCoveredInfo: return C_COCO_NOT_COVERED_INFO;
    case CocoDiagnosticSeverity::CoveredInfo: return C_COCO_COVERED_INFO;
    case CocoDiagnosticSeverity::ManuallyValidatedInfo: return C_COCO_MANUALLY_VALIDATED_INFO;
    }
    return C_TEXT;
}

class CocoDiagnostic : public Diagnostic
{
public:
    using Diagnostic::Diagnostic;
    optional<CocoDiagnosticSeverity> cocoSeverity() const
    {
        if (auto val = optionalValue<int>(severityKey))
            return Utils::make_optional(static_cast<CocoDiagnosticSeverity>(*val));
        return Utils::nullopt;
    }
};

class CocoTextMark : public TextEditor::TextMark
{
public:
    CocoTextMark(const FilePath &fileName, const CocoDiagnostic &diag, const Id &clientId)
        : TextEditor::TextMark(fileName, diag.range().start().line() + 1, clientId)
    {
        setLineAnnotation(diag.message());
        setToolTip(diag.message());
        if (optional<CocoDiagnosticSeverity> severity = diag.cocoSeverity()) {

            const TextEditor::TextStyle style = styleForSeverity(*severity);
            m_annotationColor =
                    TextEditor::TextEditorSettings::fontSettings().formatFor(style).foreground();
        }
    }

    QColor annotationColor() const override
    {
        return m_annotationColor.isValid() ? m_annotationColor
                                           : TextEditor::TextMark::annotationColor();
    }

    QColor m_annotationColor;
};

class CocoDiagnosticManager : public DiagnosticManager
{
public:
    CocoDiagnosticManager(Client *client)
        : DiagnosticManager(client)
    {
        setExtraSelectionsId("CocoExtraSelections");
    }

private:
    TextEditor::TextMark *createTextMark(const FilePath &filePath,
                                         const Diagnostic &diagnostic,
                                         bool /*isProjectFile*/) const override
    {
        const CocoDiagnostic cocoDiagnostic(diagnostic);
        if (optional<CocoDiagnosticSeverity> severity = cocoDiagnostic.cocoSeverity())
            return new CocoTextMark(filePath, cocoDiagnostic, client()->id());
        return nullptr;
    }

    QTextEdit::ExtraSelection createDiagnosticSelection(const Diagnostic &diagnostic,
                                                        QTextDocument *textDocument) const override
    {
        if (optional<CocoDiagnosticSeverity> severity = CocoDiagnostic(diagnostic).cocoSeverity()) {
            QTextCursor cursor(textDocument);
            cursor.setPosition(diagnostic.range().start().toPositionInDocument(textDocument));
            cursor.setPosition(diagnostic.range().end().toPositionInDocument(textDocument),
                               QTextCursor::KeepAnchor);

            const TextEditor::TextStyle style = styleForSeverity(*severity);
            QTextCharFormat format = TextEditor::TextEditorSettings::fontSettings()
                    .toTextCharFormat(style);
            format.clearForeground();
            return QTextEdit::ExtraSelection{cursor, format};
        }
        return {};
    }

    void setDiagnostics(const DocumentUri &uri,
                        const QList<Diagnostic> &diagnostics,
                        const Utils::optional<int> &version) override
    {
        DiagnosticManager::setDiagnostics(uri, diagnostics, version);
        showDiagnostics(uri, client()->documentVersion(uri.toFilePath()));
    }
};

DiagnosticManager *CocoLanguageClient::createDiagnosticManager()
{
    return new CocoDiagnosticManager(this);
}

void CocoLanguageClient::handleDiagnostics(const PublishDiagnosticsParams &params)
{
    using namespace TextEditor;
    Client::handleDiagnostics(params);
    TextDocument *document = documentForFilePath(params.uri().toFilePath());
    for (BaseTextEditor *editor : BaseTextEditor::textEditorsForDocument(document))
        editor->editorWidget()->addHoverHandler(hoverHandler());
}

class CocoTextDocumentCapabilities : public TextDocumentClientCapabilities
{
public:
    using TextDocumentClientCapabilities::TextDocumentClientCapabilities;
    void enableCodecoverageSupport()
    {
        JsonObject coverageSupport(QJsonObject{{"codeCoverageSupport", true}});
        insert("publishDiagnostics", coverageSupport);
    }
};

void CocoLanguageClient::initClientCapabilities()
{
    ClientCapabilities capabilities = defaultClientCapabilities();
    CocoTextDocumentCapabilities textDocumentCapabilities(
        capabilities.textDocument().value_or(TextDocumentClientCapabilities()));
    textDocumentCapabilities.enableCodecoverageSupport();
    capabilities.setTextDocument(textDocumentCapabilities);
    setClientCapabilities(capabilities);
}

void CocoLanguageClient::handleDocumentOpened(IDocument *document)
{
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
        openDocument(textDocument);
}

void CocoLanguageClient::handleDocumentClosed(IDocument *document)
{
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
        closeDocument(textDocument);
}

void CocoLanguageClient::handleEditorOpened(IEditor *editor)
{
    if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
            textEditor && hasDiagnostics(textEditor->textDocument())) {
        textEditor->editorWidget()->addHoverHandler(hoverHandler());
    }
}

} // namespace Coco
