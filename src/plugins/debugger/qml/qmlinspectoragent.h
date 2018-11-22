/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QStack>
#include <QPointer>
#include <QTimer>

#include <coreplugin/icontext.h>
#include <debugger/debuggerconstants.h>
#include <debugger/watchdata.h>
#include <qmldebug/qmlenginedebugclient.h>

namespace QmlDebug {
class QmlToolsClient;
class ObjectReference;
class FileReference;
class QmlDebugConnection;
}

namespace Debugger {
namespace Internal {

class DebuggerEngine;
class QmlEngine;

class QmlInspectorAgent : public QObject
{
    Q_OBJECT

public:
    QmlInspectorAgent(QmlEngine *engine, QmlDebug::QmlDebugConnection *connection);

    quint32 queryExpressionResult(int debugId, const QString &expression, int engineId);
    void assignValue(const WatchItem *data, const QString &expression, const QVariant &valueV);
    void updateWatchData(const WatchItem &data);
    void watchDataSelected(int id);
    void enableTools(bool enable);
    int engineId(const WatchItem *item) const;

private:
    void selectObjectsInTree(const QList<int> &debugIds);
    void addObjectWatch(int objectDebugId);

    void reloadEngines();
    void fetchObject(int debugId);

    void updateState();
    void onResult(quint32 queryId, const QVariant &value, const QByteArray &type);
    void newObject(int engineId, int objectId, int parentId);
    void onValueChanged(int debugId, const QByteArray &propertyName, const QVariant &value);

    void queryEngineContext();
    void updateObjectTree(const QmlDebug::ContextReference &contexts, int engineId = -1);
    void verifyAndInsertObjectInTree(const QmlDebug::ObjectReference &object, int engineId = -1);
    void insertObjectInTree(const QmlDebug::ObjectReference &result, int parentId);

    void buildDebugIdHashRecursive(const QmlDebug::ObjectReference &ref);
    void addWatchData(const QmlDebug::ObjectReference &obj,
                      const QString &parentIname, bool append);

    enum LogDirection {
        LogSend,
        LogReceive
    };
    void log(LogDirection direction, const QString &message);

    bool isConnected() const;
    void clearObjectTree();

    void toolsClientStateChanged(QmlDebug::QmlDebugClient::State state);

    void selectObjectsFromToolsClient(const QList<int> &debugIds);

    void onSelectActionTriggered(bool checked);
    void onShowAppOnTopChanged(bool checked);
    void onReloaded();
    void jumpToObjectDefinitionInEditor(const QmlDebug::FileReference &objSource);

    void selectObjects(const QList<int> &debugIds, const QmlDebug::FileReference &source);

    QPointer<QmlEngine> m_qmlEngine;
    QmlDebug::QmlEngineDebugClient *m_engineClient = nullptr;
    QmlDebug::QmlToolsClient *m_toolsClient = nullptr;

    quint32 m_engineQueryId = 0;

    QList<quint32> m_rootContextQueryIds;
    QHash<int, QmlDebug::ContextReference> m_rootContexts;

    QList<int> m_objectsToSelect;

    QList<quint32> m_objectTreeQueryIds;
    QStack<QPair<QmlDebug::ObjectReference, int>> m_objectStack;
    QList<QmlDebug::EngineReference> m_engines;
    QHash<int, QString> m_debugIdToIname;
    QHash<int, QmlDebug::FileReference> m_debugIdLocations;

    QList<int> m_objectWatches;
    QList<int> m_fetchDataIds;
    QTimer m_delayQueryTimer;

    // toolbar
    Core::Context m_inspectorToolsContext;
    QAction *m_selectAction = nullptr;
    QAction *m_showAppOnTopAction = nullptr;
};

} // Internal
} // Debugger
