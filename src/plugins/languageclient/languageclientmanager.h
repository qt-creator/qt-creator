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

#include "baseclient.h"
#include "languageclientsettings.h"

#include <coreplugin/id.h>

#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/textsynchronization.h>

namespace Core {
class IEditor;
class IDocument;
}

namespace ProjectExplorer { class Project; }

namespace LanguageClient {

class LanguageClientMark;

class LanguageClientManager : public QObject
{
    Q_OBJECT
public:
    ~LanguageClientManager() override;

    static void init();

    static void publishDiagnostics(const Core::Id &id,
                                   const LanguageServerProtocol::PublishDiagnosticsParams &params);

    static void removeMark(LanguageClientMark *mark);
    static void removeMarks(const Utils::FileName &fileName);
    static void removeMarks(const Utils::FileName &fileName, const Core::Id &id);
    static void removeMarks(const Core::Id &id);

    static void startClient(BaseClient *client);
    static QVector<BaseClient *> clients();

    static void addExclusiveRequest(const LanguageServerProtocol::MessageId &id, BaseClient *client);
    static void reportFinished(const LanguageServerProtocol::MessageId &id, BaseClient *byClient);

    static void deleteClient(BaseClient *client);

    static void shutdown();

    static LanguageClientManager *instance();

signals:
    void shutdownFinished();

private:
    LanguageClientManager();
    LanguageClientManager(const LanguageClientManager &other) = delete;
    LanguageClientManager(LanguageClientManager &&other) = delete;

    void editorOpened(Core::IEditor *editor);
    void editorsClosed(const QList<Core::IEditor *> editors);
    void documentContentsSaved(Core::IDocument *document);
    void documentWillSave(Core::IDocument *document);
    void findLinkAt(const Utils::FileName &filePath, const QTextCursor &cursor,
                    Utils::ProcessLinkCallback callback);

    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    QVector<BaseClient *> reachableClients();
    void sendToAllReachableServers(const LanguageServerProtocol::IContent &content);

    void clientFinished(BaseClient *client);

    bool m_shuttingDown = false;
    QVector<BaseClient *> m_clients;
    QHash<Utils::FileName, QHash<Core::Id, QVector<LanguageClientMark *>>> m_marks;
    QHash<LanguageServerProtocol::MessageId, QList<BaseClient *>> m_exclusiveRequests;

    friend class LanguageClientPlugin;
};
} // namespace LanguageClient
