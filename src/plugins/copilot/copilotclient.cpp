// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotclient.h"
#include "copilotsettings.h"
#include "copilotsuggestion.h"
#include "copilottr.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>
#include <languageserverprotocol/lsptypes.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectmanager.h>

#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <utils/checkablemessagebox.h>
#include <utils/filepath.h>
#include <utils/passworddialog.h>

#include <QGuiApplication>
#include <QInputDialog>
#include <QLoggingCategory>
#include <QTimer>
#include <QToolButton>

using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;
using namespace ProjectExplorer;
using namespace Core;

Q_LOGGING_CATEGORY(copilotClientLog, "qtc.copilot.client", QtWarningMsg)

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

    registerCustomMethod("LogMessage", [this](const LanguageServerProtocol::JsonRpcMessage &message) {
        QString msg = message.toJsonObject().value("params").toObject().value("message").toString();
        qCDebug(copilotClientLog) << message.toJsonObject()
                                         .value("params")
                                         .toObject()
                                         .value("message")
                                         .toString();

        if (msg.contains("Socket Connect returned status code,407")) {
            qCWarning(copilotClientLog) << "Proxy authentication required";
            QMetaObject::invokeMethod(this,
                                      &CopilotClient::proxyAuthenticationFailed,
                                      Qt::QueuedConnection);
        }
    });

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

    connect(this, &LanguageClient::Client::initialized, this, &CopilotClient::requestSetEditorInfo);

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
                if (!settings().autoComplete())
                    return;

                auto project = ProjectManager::projectForFile(document->filePath());
                if (!isEnabled(project))
                    return;

                auto textEditor = BaseTextEditor::currentTextEditor();
                if (!textEditor || textEditor->document() != document)
                    return;
                TextEditorWidget *widget = textEditor->editorWidget();
                if (widget->isReadOnly() || widget->multiTextCursor().hasMultipleCursors())
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

static QString currentProxyPassword;

void CopilotClient::requestSetEditorInfo()
{
    if (settings().saveProxyPassword())
        currentProxyPassword = settings().proxyPassword();

    const EditorInfo editorInfo{QCoreApplication::applicationVersion(),
                                QGuiApplication::applicationDisplayName()};
    const EditorPluginInfo editorPluginInfo{QCoreApplication::applicationVersion(),
                                            "Qt Creator Copilot plugin"};

    SetEditorInfoParams params(editorInfo, editorPluginInfo);

    if (settings().useProxy()) {
        params.setNetworkProxy(
            Copilot::NetworkProxy{settings().proxyHost(),
                                  static_cast<int>(settings().proxyPort()),
                                  settings().proxyUser(),
                                  currentProxyPassword,
                                  settings().proxyRejectUnauthorized()});
    }

    SetEditorInfoRequest request(params);
    sendMessage(request);
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
        return settings().enableCopilot();

    CopilotProjectSettings settings(project);
    return settings.isEnabled();
}

void CopilotClient::proxyAuthenticationFailed()
{
    static bool doNotAskAgain = false;

    if (m_isAskingForPassword || !settings().enableCopilot())
        return;

    m_isAskingForPassword = true;

    auto answer = Utils::PasswordDialog::getUserAndPassword(
        Tr::tr("Copilot"),
        Tr::tr("Proxy username and password required:"),
        Tr::tr("Do not ask again. This will disable Copilot for now."),
        settings().proxyUser(),
        &doNotAskAgain,
        Core::ICore::dialogParent());

    if (answer) {
        settings().proxyUser.setValue(answer->first);
        currentProxyPassword = answer->second;
    } else {
        settings().enableCopilot.setValue(false);
    }

    if (settings().saveProxyPassword())
        settings().proxyPassword.setValue(currentProxyPassword);

    settings().apply();

    m_isAskingForPassword = false;
}

} // namespace Copilot::Internal
