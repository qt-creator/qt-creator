// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocolanguageclient.h"

#include <coreplugin/editormanager/editormanager.h>
#include <languageclient/diagnosticmanager.h>
#include <languageclient/languageclienthoverhandler.h>
#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientsettings.h>
#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/initializemessages.h>
#include <projectexplorer/project.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textmark.h>
#include <utils/utilsicons.h>

#include <QGuiApplication>
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
    hoverHandler()->setPreferDiagnosticts(false);
    setActivateDocumentAutomatically(false);
    LanguageFilter allFiles;
    allFiles.filePattern = QStringList{"*"};
    setSupportedLanguage(allFiles);
    connect(EditorManager::instance(),
            &EditorManager::documentOpened,
            this,
            &CocoLanguageClient::onDocumentOpened);
    connect(EditorManager::instance(),
            &EditorManager::documentClosed,
            this,
            &CocoLanguageClient::onDocumentClosed);
    connect(EditorManager::instance(),
            &EditorManager::editorOpened,
            this,
            &CocoLanguageClient::handleEditorOpened);
    for (IDocument *openDocument : DocumentModel::openedDocuments())
        onDocumentOpened(openDocument);

    ClientInfo info;
    info.setName("CocoQtCreator");
    info.setVersion(QGuiApplication::applicationDisplayName());
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
    std::optional<CocoDiagnosticSeverity> cocoSeverity() const
    {
        if (auto val = optionalValue<int>(severityKey))
            return std::make_optional(static_cast<CocoDiagnosticSeverity>(*val));
        return std::nullopt;
    }
};

class CocoTextMark : public TextEditor::TextMark
{
public:
    CocoTextMark(TextEditor::TextDocument *doc, const CocoDiagnostic &diag, const Id &clientId)
        : TextEditor::TextMark(doc, diag.range().start().line() + 1, {"Coco", clientId})
        , m_severity(diag.cocoSeverity())
    {
        setLineAnnotation(diag.message());
        setToolTip(diag.message());
        updateAnnotationColor();
    }

    QColor annotationColor() const override
    {
        return m_annotationColor.isValid() ? m_annotationColor
                                           : TextEditor::TextMark::annotationColor();
    }

    void updateAnnotationColor()
    {
        if (m_severity) {
            const TextEditor::TextStyle style = styleForSeverity(*m_severity);
            m_annotationColor =
                    TextEditor::TextEditorSettings::fontSettings().formatFor(style).foreground();
        }
    }

    std::optional<CocoDiagnosticSeverity> m_severity;
    QColor m_annotationColor;
};

class CocoDiagnosticManager : public DiagnosticManager
{
public:
    CocoDiagnosticManager(Client *client)
        : DiagnosticManager(client)
    {
        connect(TextEditor::TextEditorSettings::instance(),
                &TextEditor::TextEditorSettings::fontSettingsChanged,
                this,
                &CocoDiagnosticManager::fontSettingsChanged);
        setExtraSelectionsId("CocoExtraSelections");
    }

private:
    void fontSettingsChanged()
    {
        forAllMarks([](TextEditor::TextMark *mark){
            static_cast<CocoTextMark *>(mark)->updateAnnotationColor();
            mark->updateMarker();
        });
    }

    TextEditor::TextMark *createTextMark(TextEditor::TextDocument *doc,
                                         const Diagnostic &diagnostic,
                                         bool /*isProjectFile*/) const override
    {
        const CocoDiagnostic cocoDiagnostic(diagnostic);
        if (std::optional<CocoDiagnosticSeverity> severity = cocoDiagnostic.cocoSeverity())
            return new CocoTextMark(doc, cocoDiagnostic, client()->id());
        return nullptr;
    }

    QTextEdit::ExtraSelection createDiagnosticSelection(const Diagnostic &diagnostic,
                                                        QTextDocument *textDocument) const override
    {
        if (std::optional<CocoDiagnosticSeverity> severity = CocoDiagnostic(diagnostic)
                                                                 .cocoSeverity()) {
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

    void setDiagnostics(const FilePath &filePath,
                        const QList<Diagnostic> &diagnostics,
                        const std::optional<int> &version) override
    {
        DiagnosticManager::setDiagnostics(filePath, diagnostics, version);
        showDiagnostics(filePath, client()->documentVersion(filePath));
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
    TextDocument *document = documentForFilePath(serverUriToHostPath(params.uri()));
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

void CocoLanguageClient::onDocumentOpened(IDocument *document)
{
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
        openDocument(textDocument);
}

void CocoLanguageClient::onDocumentClosed(IDocument *document)
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
