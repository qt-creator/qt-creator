/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef QMLJSCLIENTPROXY_H
#define QMLJSCLIENTPROXY_H

#include "qmljsinspectorplugin.h"
#include "qmljsprivateapi.h"
#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QDeclarativeEngineDebug)
QT_FORWARD_DECLARE_CLASS(QDeclarativeDebugConnection)
QT_FORWARD_DECLARE_CLASS(QDeclarativeDebugExpressionQuery)

namespace Debugger {
    class DebuggerRunControl;
}

namespace QmlJSInspector {
namespace Internal {

class InspectorPlugin;

class ClientProxy : public QObject
{
    Q_OBJECT

public:
    static ClientProxy *instance();

    QDeclarativeDebugExpressionQuery *setBindingForObject(int objectDebugId,
                                                          const QString &propertyName,
                                                          const QVariant &value,
                                                          bool isLiteralValue);

    // returns the object references for the given url.
    QList<QDeclarativeDebugObjectReference> objectReferences(const QUrl &url) const;

    bool isConnected() const;
    bool isUnconnected() const;

    void setSelectedItemByObjectId(int engineId, const QDeclarativeDebugObjectReference &objectRef);

    bool connectToViewer(const QString &host, quint16 port);
    void disconnectFromViewer();

    QList<QDeclarativeDebugEngineReference> engines() const;

signals:
    void objectTreeUpdated(const QDeclarativeDebugObjectReference &rootObject);
    void connectionStatusMessage(const QString &text);

    void aboutToReloadEngines();
    void enginesChanged();

    void selectedItemsChanged(const QList<QDeclarativeDebugObjectReference> &selectedItems);

    void connected(QDeclarativeEngineDebug *client);
    void aboutToDisconnect();
    void disconnected();

public slots:
    void queryEngineContext(int id);
    void reloadQmlViewer(int engineId);

private slots:
    void contextChanged();
    void connectionStateChanged();
    void connectionError();

    void updateEngineList();
    void objectTreeFetched(QDeclarativeDebugQuery::State state = QDeclarativeDebugQuery::Completed);

private:
    void reloadEngines();
    QList<QDeclarativeDebugObjectReference> objectReferences(const QUrl &url, const QDeclarativeDebugObjectReference &objectRef) const;

private:
    explicit ClientProxy(QObject *parent = 0);
    Q_DISABLE_COPY(ClientProxy);

    static ClientProxy *m_instance;

    QDeclarativeDebugConnection *m_conn;
    QDeclarativeEngineDebug *m_client;

    QDeclarativeDebugEnginesQuery *m_engineQuery;
    QDeclarativeDebugRootContextQuery *m_contextQuery;
    QDeclarativeDebugObjectQuery *m_objectTreeQuery;

    QDeclarativeDebugObjectReference m_rootObject;
    QList<QDeclarativeDebugEngineReference> m_engines;

    Debugger::DebuggerRunControl *m_debuggerRunControl;
    friend class QmlJSInspector::Internal::InspectorPlugin;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSCLIENTPROXY_H
