/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLINSPECTORADAPTER_H
#define QMLINSPECTORADAPTER_H

#include "debuggerconstants.h"

#include <QObject>
#include <QStringList>

#include <coreplugin/icontext.h>
#include <qmldebug/qmldebugclient.h>
#include <qmljs/qmljsdocument.h>

namespace Core {
class IEditor;
}

namespace QmlDebug {
class BaseEngineDebugClient;
class BaseToolsClient;
class ObjectReference;
class FileReference;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class WatchTreeView;
class QmlAdapter;
class QmlInspectorAgent;
class QmlLiveTextPreview;

class QmlInspectorAdapter : public QObject
{
    Q_OBJECT

public:
    QmlInspectorAdapter(QmlAdapter *debugAdapter, DebuggerEngine *engine,
                        QObject *parent = 0);
    ~QmlInspectorAdapter();

    QmlDebug::BaseEngineDebugClient *engineClient() const;
    QmlDebug::BaseToolsClient *toolsClient() const;
    QmlInspectorAgent *agent() const;

    int currentSelectedDebugId() const;
    QString currentSelectedDisplayName() const;

signals:
    void expressionResult();

private slots:
    void onEngineStateChanged(const Debugger::DebuggerState);

    void clientStatusChanged(QmlDebug::ClientStatus status);
    void toolsClientStatusChanged(QmlDebug::ClientStatus status);
    void engineClientStatusChanged(QmlDebug::ClientStatus status);

    void selectObjectsFromToolsClient(const QList<int> &debugIds);
    void onObjectFetched(const QmlDebug::ObjectReference &ref);

    void createPreviewForEditor(Core::IEditor *newEditor);
    void removePreviewForEditor(Core::IEditor *editor);
    void updatePendingPreviewDocuments(QmlJS::Document::Ptr doc);

    void onSelectActionTriggered(bool checked);
    void onZoomActionTriggered(bool checked);
    void onShowAppOnTopChanged(bool checked);
    void onUpdateOnSaveChanged(bool checked);
    void onReload();
    void onReloaded();
    void onDestroyedObject(int);
    void jumpToObjectDefinitionInEditor(const QmlDebug::FileReference &objSource, int debugId = -1);

private:
    void setActiveEngineClient(QmlDebug::BaseEngineDebugClient *client);

    void initializePreviews();
    void showConnectionStatusMessage(const QString &message);

    enum SelectionTarget { NoTarget, ToolTarget, EditorTarget };
    void selectObject(
            const QmlDebug::ObjectReference &objectReference,
            SelectionTarget target);
    void deletePreviews();

    void enableTools(const bool enable);

    QmlAdapter *m_debugAdapter;
    DebuggerEngine *m_engine; // Master Engine
    QmlDebug::BaseEngineDebugClient *m_engineClient;
    QHash<QString, QmlDebug::BaseEngineDebugClient*> m_engineClients;
    QmlDebug::BaseToolsClient *m_toolsClient;
    QmlInspectorAgent *m_agent;

    SelectionTarget m_targetToSync;
    int m_debugIdToSelect;

    int m_currentSelectedDebugId;
    QString m_currentSelectedDebugName;

    // Qml/JS editor integration
    bool m_listeningToEditorManager;
    QHash<QString, QmlLiveTextPreview *> m_textPreviews;
    QmlJS::Snapshot m_loadedSnapshot; //the snapshot loaded by the viewer
    QStringList m_pendingPreviewDocumentNames;

    // toolbar
    bool m_toolsClientConnected;
    Core::Context m_inspectorToolsContext;
    QAction *m_selectAction;
    QAction *m_zoomAction;
    QAction *m_showAppOnTopAction;
    QAction *m_updateOnSaveAction;

    bool m_engineClientConnected;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLINSPECTORADAPTER_H
