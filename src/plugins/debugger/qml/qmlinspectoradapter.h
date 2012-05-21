/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLINSPECTORADAPTER_H
#define QMLINSPECTORADAPTER_H

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
}

namespace Debugger {
namespace Internal {

class WatchTreeView;
class QmlAdapter;
class QmlEngine;
class QmlInspectorAgent;
class QmlLiveTextPreview;

class QmlInspectorAdapter : public QObject
{
    Q_OBJECT

public:
    QmlInspectorAdapter(QmlAdapter *debugAdapter, QmlEngine *engine,
                        QObject *parent = 0);
    ~QmlInspectorAdapter();

    QmlDebug::BaseEngineDebugClient *engineClient() const;
    QmlDebug::BaseToolsClient *toolsClient() const;
    QmlInspectorAgent *agent() const;

    int currentSelectedDebugId() const;
    QString currentSelectedDisplayName() const;

signals:
    void expressionResult();
    void selectionChanged();

private slots:
    void clientStatusChanged(QmlDebug::ClientStatus status);
    void toolsClientStatusChanged(QmlDebug::ClientStatus status);
    void engineClientStatusChanged(QmlDebug::ClientStatus status);

    void selectObjectsFromEditor(const QList<int> &debugIds);
    void selectObjectsFromToolsClient(const QList<int> &debugIds);
    void onObjectFetched(const QmlDebug::ObjectReference &ref);
    void onObjectTreeUpdated();

    void createPreviewForEditor(Core::IEditor *newEditor);
    void removePreviewForEditor(Core::IEditor *editor);
    void updatePendingPreviewDocuments(QmlJS::Document::Ptr doc);

    void onSelectActionTriggered(bool checked);
    void onZoomActionTriggered(bool checked);
    void onShowAppOnTopChanged(const QVariant &value);
    void onUpdateOnSaveChanged(const QVariant &value);
    void onReload();
    void onReloaded();
    void onDestroyedObject();

private:
    void setActiveEngineClient(QmlDebug::BaseEngineDebugClient *client);

    void initializePreviews();
    void showConnectionStatusMessage(const QString &message);

    void gotoObjectReferenceDefinition(const QmlDebug::ObjectReference &obj);
    QmlDebug::ObjectReference objectReferenceForLocation(
            const QString &fileName, int cursorPosition = -1) const;

    enum SelectionTarget { NoTarget, ToolTarget, EditorTarget };
    void selectObject(
            const QmlDebug::ObjectReference &objectReference,
            SelectionTarget target);
    void deletePreviews();


    QmlAdapter *m_debugAdapter;
    QmlEngine *m_engine;
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
    bool m_selectionCallbackExpected;
    bool m_cursorPositionChangedExternally;

    // toolbar
    bool m_toolsClientConnected;
    Core::Context m_inspectorToolsContext;
    QAction *m_selectAction;
    QAction *m_zoomAction;

    bool m_engineClientConnected;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLINSPECTORADAPTER_H
