/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLINSPECTORAGENT_H
#define QMLINSPECTORAGENT_H

#include <QStack>
#include <QPointer>
#include <QTimer>

#include <coreplugin/icontext.h>
#include <debugger/debuggerconstants.h>
#include <qmldebug/baseenginedebugclient.h>

namespace QmlDebug {
class BaseEngineDebugClient;
class BaseToolsClient;
class ObjectReference;
class FileReference;
class QmlDebugConnection;
}

namespace Debugger {
namespace Internal {

class DebuggerEngine;
class QmlEngine;
class WatchData;

//map <filename, editorRevision> -> <lineNumber, columnNumber> -> debugId
typedef
QHash<QPair<QString, int>, QHash<QPair<int, int>, QList<int> > > DebugIdHash;

class QmlInspectorAgent : public QObject
{
    Q_OBJECT
public:
    QmlInspectorAgent(QmlEngine *engine, QmlDebug::QmlDebugConnection *connection);

    void fetchObject(int debugId);
    quint32 queryExpressionResult(int debugId, const QString &expression);

    void assignValue(const WatchData *data, const QString &expression, const QVariant &valueV);
    void updateWatchData(const WatchData &data);
    void watchDataSelected(qint64 id);
    bool selectObjectInTree(int debugId);
    void addObjectWatch(int objectDebugId);

    QmlDebug::ObjectReference objectForId(int objectDebugId) const;
    QString displayName(int objectDebugId) const;
    void reloadEngines();

    QmlDebug::BaseToolsClient *toolsClient() const;

private:
    void updateState();
    void onResult(quint32 queryId, const QVariant &value, const QByteArray &type);
    void newObject(int engineId, int objectId, int parentId);
    void onValueChanged(int debugId, const QByteArray &propertyName, const QVariant &value);

    void queryEngineContext();
    void updateObjectTree(const QmlDebug::ContextReference &context);
    void verifyAndInsertObjectInTree(const QmlDebug::ObjectReference &object);
    void insertObjectInTree(const QmlDebug::ObjectReference &result);

    void buildDebugIdHashRecursive(const QmlDebug::ObjectReference &ref);
    void addWatchData(const QmlDebug::ObjectReference &obj,
                      const QByteArray &parentIname, bool append);

    enum LogDirection {
        LogSend,
        LogReceive
    };
    void log(LogDirection direction, const QString &message);

    bool isConnected() const;
    void clearObjectTree();

    void onEngineStateChanged(const Debugger::DebuggerState);

    void clientStateChanged(QmlDebug::QmlDebugClient::State state);
    void toolsClientStateChanged(QmlDebug::QmlDebugClient::State state);
    void engineClientStateChanged(QmlDebug::QmlDebugClient::State state);

    void selectObjectsFromToolsClient(const QList<int> &debugIds);

    void onSelectActionTriggered(bool checked);
    void onZoomActionTriggered(bool checked);
    void onShowAppOnTopChanged(bool checked);
    void onReloaded();
    void jumpToObjectDefinitionInEditor(const QmlDebug::FileReference &objSource, int debugId = -1);

    void setActiveEngineClient(QmlDebug::BaseEngineDebugClient *client);

    enum SelectionTarget { NoTarget, ToolTarget, EditorTarget };
    void selectObject(
            const QmlDebug::ObjectReference &objectReference,
            SelectionTarget target);

    void enableTools(const bool enable);

private:
    QPointer<QmlEngine> m_qmlEngine;
    QmlDebug::BaseEngineDebugClient *m_engineClient;

    quint32 m_engineQueryId;
    quint32 m_rootContextQueryId;
    int m_objectToSelect;
    QList<quint32> m_objectTreeQueryIds;
    QStack<QmlDebug::ObjectReference> m_objectStack;
    QmlDebug::EngineReference m_engine;
    QHash<int, QByteArray> m_debugIdToIname;
    QHash<int, QmlDebug::FileReference> m_debugIdLocations;
    DebugIdHash m_debugIdHash;

    QList<int> m_objectWatches;
    QList<int> m_fetchDataIds;
    QTimer m_delayQueryTimer;

    DebuggerEngine *m_masterEngine;
    QHash<QString, QmlDebug::BaseEngineDebugClient*> m_engineClients;
    QmlDebug::BaseToolsClient *m_toolsClient;

    SelectionTarget m_targetToSync;
    int m_debugIdToSelect;

    int m_currentSelectedDebugId;
    QString m_currentSelectedDebugName;

    // toolbar
    bool m_toolsClientConnected;
    Core::Context m_inspectorToolsContext;
    QAction *m_selectAction;
    QAction *m_zoomAction;
    QAction *m_showAppOnTopAction;

    bool m_engineClientConnected;
};

} // Internal
} // Debugger

#endif // QMLINSPECTORAGENT_H
