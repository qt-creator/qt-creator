// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotclient.h"
#include "copilotconstants.h"
#include "copilotsettings.h"
#include "copilotsuggestion.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/projectmanager.h>

#include <utils/filepath.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <languageserverprotocol/lsptypes.h>

#include <QTimer>
#include <QToolButton>

using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;
using namespace ProjectExplorer;
using namespace Core;

namespace Copilot::Internal {

static LanguageClient::BaseClientInterface *clientInterface(const FilePath &nodePath,
                                                            const FilePath &distPath)
{
    CommandLine cmd{nodePath, {distPath.toFSPathString()}};

    const auto interface = new LanguageClient::StdIOClientInterface;
    interface->setCommandLine(cmd);
    return interface;
}

CopilotClient::CopilotClient(const FilePath &nodePath, const FilePath &distPath)
    : LanguageClient::Client(clientInterface(nodePath, distPath))
{
    setName("Copilot");
    LanguageClient::LanguageFilter langFilter;

    langFilter.filePattern = {"*"};

    setSupportedLanguage(langFilter);
    start();

    auto openDoc = [this](IDocument *document) {
        if (auto *textDocument = qobject_cast<TextDocument *>(document))
            openDocument(textDocument);
    };

    connect(EditorManager::instance(), &EditorManager::documentOpened, this, openDoc);
    connect(EditorManager::instance(),
            &EditorManager::documentClosed,
            this,
            [this](IDocument *document) {
                if (auto textDocument = qobject_cast<TextDocument *>(document))
                    closeDocument(textDocument);
            });

    for (IDocument *doc : DocumentModel::openedDocuments())
        openDoc(doc);
}

CopilotClient::~CopilotClient()
{
    for (IEditor *editor : DocumentModel::editorsForOpenedDocuments()) {
        if (auto textEditor = qobject_cast<BaseTextEditor *>(editor))
            textEditor->editorWidget()->removeHoverHandler(&m_hoverHandler);
    }
}

void CopilotClient::openDocument(TextDocument *document)
{
    auto project = ProjectManager::projectForFile(document->filePath());
    if (!isEnabled(project))
        return;

    Client::openDocument(document);
    connect(document,
            &TextDocument::contentsChangedWithPosition,
            this,
            [this, document](int position, int charsRemoved, int charsAdded) {
                Q_UNUSED(charsRemoved)
                if (!CopilotSettings::instance().autoComplete())
                    return;

                auto project = ProjectManager::projectForFile(document->filePath());
                if (!isEnabled(project))
                    return;

                auto textEditor = BaseTextEditor::currentTextEditor();
                if (!textEditor || textEditor->document() != document)
                    return;
                TextEditorWidget *widget = textEditor->editorWidget();
                if (widget->multiTextCursor().hasMultipleCursors())
                    return;
                const int cursorPosition = widget->textCursor().position();
                if (cursorPosition < position || cursorPosition > position + charsAdded)
                    return;
                scheduleRequest(widget);
            });
}

void CopilotClient::scheduleRequest(TextEditorWidget *editor)
{
    cancelRunningRequest(editor);

    if (!m_scheduledRequests.contains(editor)) {
        auto timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, editor]() {
            if (m_scheduledRequests[editor].cursorPosition == editor->textCursor().position())
                requestCompletions(editor);
        });
        connect(editor, &TextEditorWidget::destroyed, this, [this, editor]() {
            delete m_scheduledRequests.take(editor).timer;
            cancelRunningRequest(editor);
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
    auto project = ProjectManager::projectForFile(editor->textDocument()->filePath());

    if (!isEnabled(project))
        return;

    Utils::MultiTextCursor cursor = editor->multiTextCursor();
    if (cursor.hasMultipleCursors() || cursor.hasSelection() || editor->suggestionVisible())
        return;

    const Utils::FilePath filePath = editor->textDocument()->filePath();
    GetCompletionRequest request{
        {TextDocumentIdentifier(hostPathToServerUri(filePath)),
         documentVersion(filePath),
         Position(cursor.mainCursor())}};
    request.setResponseCallback([this, editor = QPointer<TextEditorWidget>(editor)](
                                    const GetCompletionRequest::Response &response) {
        QTC_ASSERT(editor, return);
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

    int requestPosition = -1;
    if (const auto requestParams = m_runningRequests.take(editor).params())
        requestPosition = requestParams->position().toPositionInDocument(editor->document());

    const Utils::MultiTextCursor cursors = editor->multiTextCursor();
    if (cursors.hasMultipleCursors())
        return;

    if (cursors.hasSelection() || cursors.mainCursor().position() != requestPosition)
        return;

    if (const std::optional<GetCompletionResponse> result = response.result()) {
        auto isValidCompletion = [](const Completion &completion) {
            return completion.isValid() && !completion.text().trimmed().isEmpty();
        };
        QList<Completion> completions = Utils::filtered(result->completions().toListOrEmpty(),
                                                              isValidCompletion);

        // remove trailing whitespaces from the end of the completions
        for (Completion &completion : completions) {
            const LanguageServerProtocol::Range range = completion.range();
            if (range.start().line() != range.end().line())
                continue; // do not remove trailing whitespaces for multi-line replacements

            const QString completionText = completion.text();
            const int end = int(completionText.size()) - 1; // empty strings have been removed above
            int delta = 0;
            while (delta <= end && completionText[end - delta].isSpace())
                ++delta;

            if (delta > 0)
                completion.setText(completionText.chopped(delta));
        }
        if (completions.isEmpty())
            return;
        editor->insertSuggestion(
            std::make_unique<CopilotSuggestion>(completions, editor->document()));
        editor->addHoverHandler(&m_hoverHandler);
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

bool CopilotClient::canOpenProject(Project *project)
{
    return isEnabled(project);
}

bool CopilotClient::isEnabled(Project *project)
{
    if (!project)
        return CopilotSettings::instance().enableCopilot();

    CopilotProjectSettings settings(project);
    return settings.isEnabled();
}

} // namespace Copilot::Internal
