// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "copilotclient.h"

#include "copilotsettings.h"
#include "documentwatcher.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>

#include <coreplugin/editormanager/editormanager.h>

#include <utils/filepath.h>

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

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::documentOpened,
            this,
            [this](Core::IDocument *document) {
                TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(
                    document);
                if (!textDocument)
                    return;

                openDocument(textDocument);

                m_documentWatchers.emplace(textDocument->filePath(),
                                           std::make_unique<DocumentWatcher>(this, textDocument));
            });

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::documentClosed,
            this,
            [this](Core::IDocument *document) {
                auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
                if (!textDocument)
                    return;

                closeDocument(textDocument);
                m_documentWatchers.erase(textDocument->filePath());
            });

    currentInstance = this;
}

void CopilotClient::requestCompletion(
    const Utils::FilePath &path,
    int version,
    LanguageServerProtocol::Position position,
    std::function<void(const GetCompletionRequest::Response &response)> callback)
{
    GetCompletionRequest request{
        {LanguageServerProtocol::TextDocumentIdentifier(hostPathToServerUri(path)),
         version,
         position}};
    request.setResponseCallback(callback);

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

} // namespace Copilot::Internal
