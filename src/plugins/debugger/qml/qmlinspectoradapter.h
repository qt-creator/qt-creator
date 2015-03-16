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

#ifndef QMLINSPECTORADAPTER_H
#define QMLINSPECTORADAPTER_H

#include <debugger/debuggerconstants.h>

#include <coreplugin/icontext.h>
#include <qmldebug/qmldebugclient.h>

namespace QmlDebug {
class BaseEngineDebugClient;
class BaseToolsClient;
class ObjectReference;
class FileReference;
}

namespace Debugger {
namespace Internal {

class DebuggerEngine;
class QmlAdapter;
class QmlInspectorAgent;

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

    void clientStateChanged(QmlDebug::QmlDebugClient::State state);
    void toolsClientStateChanged(QmlDebug::QmlDebugClient::State state);
    void engineClientStateChanged(QmlDebug::QmlDebugClient::State state);

    void selectObjectsFromToolsClient(const QList<int> &debugIds);
    void onObjectFetched(const QmlDebug::ObjectReference &ref);

    void onSelectActionTriggered(bool checked);
    void onZoomActionTriggered(bool checked);
    void onShowAppOnTopChanged(bool checked);
    void onReloaded();
    void onDestroyedObject(int);
    void jumpToObjectDefinitionInEditor(const QmlDebug::FileReference &objSource, int debugId = -1);

private:
    void setActiveEngineClient(QmlDebug::BaseEngineDebugClient *client);

    void showConnectionStateMessage(const QString &message);

    enum SelectionTarget { NoTarget, ToolTarget, EditorTarget };
    void selectObject(
            const QmlDebug::ObjectReference &objectReference,
            SelectionTarget target);

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

    // toolbar
    bool m_toolsClientConnected;
    Core::Context m_inspectorToolsContext;
    QAction *m_selectAction;
    QAction *m_zoomAction;
    QAction *m_showAppOnTopAction;

    bool m_engineClientConnected;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLINSPECTORADAPTER_H
