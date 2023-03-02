// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotclient.h"

#include "copilotsettings.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>

#include <coreplugin/editormanager/editormanager.h>

#include <utils/filepath.h>

#include <texteditor/texteditor.h>

#include <languageserverprotocol/lsptypes.h>

#include <QTimer>

using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;

namespace Copilot::Internal {

static LanguageClient::BaseClientInterface *clientInterface()
{
    const FilePath nodePath = CopilotSettings::instance().nodeJsPath.filePath();
    const FilePath distPath = CopilotSettings::instance().distPath.filePath();

    CommandLine cmd{nodePath, {distPath.toFSPathString()}};

    const auto interface = new LanguageClient::StdIOClientInterface;
    interface->setCommandLine(cmd);
    return interface;
}

static CopilotClient *currentInstance = nullptr;

CopilotClient *CopilotClient::instance()
{
    return currentInstance;
}

CopilotClient::CopilotClient()
    : LanguageClient::Client(clientInterface())
{
    setName("Copilot");
    LanguageClient::LanguageFilter langFilter;

    langFilter.filePattern = {"*"};

    setSupportedLanguage(langFilter);
    start();

    auto openDoc = [this](Core::IDocument *document) {
        if (auto *textDocument = qobject_cast<TextDocument *>(document))
            openDocument(textDocument);
    };

    connect(Core::EditorManager::instance(), &Core::EditorManager::documentOpened, this, openDoc);
    connect(Core::EditorManager::instance(),
            &Core::EditorManager::documentClosed,
            this,
            [this](Core::IDocument *document) {
                if (auto textDocument = qobject_cast<TextDocument *>(document))
                    closeDocument(textDocument);
            });

    for (Core::IDocument *doc : Core::DocumentModel::openedDocuments())
        openDoc(doc);
    currentInstance = this;
}

void CopilotClient::openDocument(TextDocument *document)
{
    Client::openDocument(document);
    connect(document,
            &TextDocument::contentsChangedWithPosition,
            this,
            [this, document](int position, int charsRemoved, int charsAdded) {
                auto textEditor = BaseTextEditor::currentTextEditor();
                if (!textEditor || textEditor->document() != document)
                    return;
                TextEditorWidget *widget = textEditor->editorWidget();
                if (widget->multiTextCursor().hasMultipleCursors())
                    return;
                if (widget->textCursor().position() != (position + charsAdded))
                    return;
                scheduleRequest(textEditor->editorWidget());
            });
}

void CopilotClient::scheduleRequest(TextEditorWidget *editor)
{
    cancelRunningRequest(editor);

    if (!m_scheduledRequests.contains(editor)) {
        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, editor]() { requestCompletions(editor); });
        connect(editor, &TextEditorWidget::destroyed, this, [this, editor]() {
            m_scheduledRequests.remove(editor);
        });
        connect(editor, &TextEditorWidget::cursorPositionChanged, this, [this, editor] {
            cancelRunningRequest(editor);
        });
        m_scheduledRequests.insert(editor, {editor->textCursor().position(), timer});
    } else {
        m_scheduledRequests[editor].cursorPosition = editor->textCursor().position();
    }
    m_scheduledRequests[editor].timer->start(500);
}

void CopilotClient::requestCompletions(TextEditorWidget *editor)
{
    Utils::MultiTextCursor cursor = editor->multiTextCursor();
    if (cursor.hasMultipleCursors() || cursor.hasSelection())
        return;

    if (m_scheduledRequests[editor].cursorPosition != cursor.mainCursor().position())
        return;

    const Utils::FilePath filePath = editor->textDocument()->filePath();
    GetCompletionRequest request{
        {TextDocumentIdentifier(hostPathToServerUri(filePath)),
         documentVersion(filePath),
         Position(cursor.mainCursor())}};
    request.setResponseCallback([this, editor = QPointer<TextEditorWidget>(editor)](
                                    const GetCompletionRequest::Response &response) {
        if (editor)
            handleCompletions(response, editor);
    });
    m_runningRequests[editor] = request;
    sendMessage(request);
}

void CopilotClient::handleCompletions(const GetCompletionRequest::Response &response,
                                      TextEditorWidget *editor)
{
    if (response.error())
        log(*response.error());

    Utils::MultiTextCursor cursor = editor->multiTextCursor();
    if (cursor.hasMultipleCursors() || cursor.hasSelection())
        return;

    if (const std::optional<GetCompletionResponse> result = response.result()) {
        LanguageClientArray<Completion> completions = result->completions();
        if (completions.isNull() || completions.toList().isEmpty())
            return;

        const Completion firstCompletion = completions.toList().first();
        const QString content = firstCompletion.text().mid(firstCompletion.position().character());

        editor->insertSuggestion(content);
    }
}

void CopilotClient::cancelRunningRequest(TextEditor::TextEditorWidget *editor)
{
    auto it = m_runningRequests.find(editor);
    if (it == m_runningRequests.end())
        return;
    cancelRequest(it->id());
    m_runningRequests.erase(it);
}

void CopilotClient::requestCheckStatus(
    bool localChecksOnly, std::function<void(const CheckStatusRequest::Response &response)> callback)
{
    CheckStatusRequest request{localChecksOnly};
    request.setResponseCallback(callback);

    sendMessage(request);
}

void CopilotClient::requestSignOut(
    std::function<void(const SignOutRequest::Response &response)> callback)
{
    SignOutRequest request;
    request.setResponseCallback(callback);

    sendMessage(request);
}

void CopilotClient::requestSignInInitiate(
    std::function<void(const SignInInitiateRequest::Response &response)> callback)
{
    SignInInitiateRequest request;
    request.setResponseCallback(callback);

    sendMessage(request);
}

void CopilotClient::requestSignInConfirm(
    const QString &userCode,
    std::function<void(const SignInConfirmRequest::Response &response)> callback)
{
    SignInConfirmRequest request(userCode);
    request.setResponseCallback(callback);

    sendMessage(request);
}

} // namespace Copilot::Internal
