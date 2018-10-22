/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "dynamiccapabilities.h"
#include "languageclientcodeassist.h"
#include "languageclientsettings.h"

#include <coreplugin/id.h>
#include <coreplugin/messagemanager.h>
#include <utils/link.h>

#include <languageserverprotocol/initializemessages.h>
#include <languageserverprotocol/shutdownmessages.h>
#include <languageserverprotocol/textsynchronization.h>
#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/client.h>
#include <languageserverprotocol/languagefeatures.h>

#include <QBuffer>
#include <QHash>
#include <QProcess>
#include <QJsonDocument>
#include <QTextCursor>

namespace Core { class IDocument; }
namespace ProjectExplorer { class Project; }
namespace TextEditor
{
    class TextDocument;
    class TextEditorWidget;
}

namespace LanguageClient {

class BaseClient : public QObject
{
    Q_OBJECT

public:
    BaseClient();
     ~BaseClient() override;

    BaseClient(const BaseClient &) = delete;
    BaseClient(BaseClient &&) = delete;
    BaseClient &operator=(const BaseClient &) = delete;
    BaseClient &operator=(BaseClient &&) = delete;

    enum State {
        Uninitialized,
        InitializeRequested,
        Initialized,
        ShutdownRequested,
        Shutdown,
        Error
    };

    void initialize();
    void shutdown();
    State state() const;
    bool reachable() const { return m_state == Initialized; }

    // document synchronization
    void openDocument(Core::IDocument *document);
    void closeDocument(const LanguageServerProtocol::DidCloseTextDocumentParams &params);
    bool documentOpen(const LanguageServerProtocol::DocumentUri &uri) const;
    void documentContentsSaved(Core::IDocument *document);
    void documentWillSave(Core::IDocument *document);
    void documentContentsChanged(Core::IDocument *document);
    void registerCapabilities(const QList<LanguageServerProtocol::Registration> &registrations);
    void unregisterCapabilities(const QList<LanguageServerProtocol::Unregistration> &unregistrations);
    bool findLinkAt(LanguageServerProtocol::GotoDefinitionRequest &request);
    void requestDocumentSymbols(TextEditor::TextDocument *document);
    void cursorPositionChanged(TextEditor::TextEditorWidget *widget);

    // workspace control
    void projectOpened(ProjectExplorer::Project *project);
    void projectClosed(ProjectExplorer::Project *project);

    void sendContent(const LanguageServerProtocol::IContent &content);
    void sendContent(const LanguageServerProtocol::DocumentUri &uri,
                     const LanguageServerProtocol::IContent &content);
    void cancelRequest(const LanguageServerProtocol::MessageId &id);

    void setSupportedLanguage(const LanguageFilter &filter);
    bool isSupportedDocument(const Core::IDocument *document) const;

    void setName(const QString &name) { m_displayName = name; }
    QString name() const { return m_displayName; }

    Core::Id id() const { return m_id; }

    bool needsRestart(const BaseSettings *) const;

    virtual bool start() { return true; }
    virtual bool reset();

    void log(const QString &message,
             Core::MessageManager::PrintToOutputPaneFlag flag = Core::MessageManager::NoModeSwitch);

signals:
    void initialized(LanguageServerProtocol::ServerCapabilities capabilities);
    void finished();

protected:
    void setError(const QString &message);
    virtual void sendData(const QByteArray &data) = 0;
    void parseData(const QByteArray &data);

private:
    void handleResponse(const LanguageServerProtocol::MessageId &id, const QByteArray &content,
                        QTextCodec *codec);
    void handleMethod(const QString &method, LanguageServerProtocol::MessageId id,
                      const LanguageServerProtocol::IContent *content);

    void intializeCallback(const LanguageServerProtocol::InitializeResponse &initResponse);
    void shutDownCallback(const LanguageServerProtocol::ShutdownResponse &shutdownResponse);
    bool sendWorkspceFolderChanges() const;
    void log(const LanguageServerProtocol::ShowMessageParams &message,
             Core::MessageManager::PrintToOutputPaneFlag flag = Core::MessageManager::NoModeSwitch);

    void showMessageBox(const LanguageServerProtocol::ShowMessageRequestParams &message,
                        const LanguageServerProtocol::MessageId &id);

    using ContentHandler = std::function<void(const QByteArray &, QTextCodec *, QString &,
                                              LanguageServerProtocol::ResponseHandlers,
                                              LanguageServerProtocol::MethodHandler)>;

    State m_state = Uninitialized;
    QHash<LanguageServerProtocol::MessageId, LanguageServerProtocol::ResponseHandler> m_responseHandlers;
    QHash<QByteArray, ContentHandler> m_contentHandler;
    QBuffer m_buffer;
    QString m_displayName;
    LanguageFilter m_languagFilter;
    QList<Utils::FileName> m_openedDocument;
    Core::Id m_id;
    LanguageServerProtocol::ServerCapabilities m_serverCapabilities;
    DynamicCapabilities m_dynamicCapabilities;
    LanguageClientCompletionAssistProvider m_completionProvider;
    QSet<TextEditor::TextDocument *> m_resetCompletionProvider;
    LanguageServerProtocol::BaseMessage m_currentMessage;
    QHash<LanguageServerProtocol::DocumentUri, LanguageServerProtocol::MessageId> m_highlightRequests;
    int m_restartsLeft = 5;
};

class StdIOClient : public BaseClient
{
    Q_OBJECT
public:
    StdIOClient(const QString &executable, const QString &arguments);
    ~StdIOClient() override;

    StdIOClient() = delete;
    StdIOClient(const StdIOClient &) = delete;
    StdIOClient(StdIOClient &&) = delete;
    StdIOClient &operator=(const StdIOClient &) = delete;
    StdIOClient &operator=(StdIOClient &&) = delete;

    bool needsRestart(const StdIOSettings *settings);

    bool start() override;

    void setWorkingDirectory(const QString &workingDirectory);

protected:
    void sendData(const QByteArray &data) final;
    QProcess m_process;

private:
    void readError();
    void readOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    const QString m_executable;
    const QString m_arguments;
};

} // namespace LanguageClient
