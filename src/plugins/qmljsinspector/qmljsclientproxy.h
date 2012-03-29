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
#ifndef QMLJSCLIENTPROXY_H
#define QMLJSCLIENTPROXY_H

#include "qmljsinspectorplugin.h"
#include "qmljsprivateapi.h"
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Debugger {
class QmlAdapter;
}

namespace QmlJSInspector {

//map <filename, editorRevision> -> <lineNumber, columnNumber> -> debugIds
typedef QHash<QPair<QString, int>, QHash<QPair<int, int>, QList<int> > > DebugIdHash;

namespace Internal {

class InspectorPlugin;
class QmlJSInspectorClient;

class ClientProxy : public QObject
{
    Q_OBJECT

public:
    explicit ClientProxy(Debugger::QmlAdapter *adapter, QObject *parent = 0);
    ~ClientProxy();

    quint32 setBindingForObject(int objectDebugId,
                             const QString &propertyName,
                             const QVariant &value,
                             bool isLiteralValue,
                             QString source,
                             int line);

    quint32 setMethodBodyForObject(int objectDebugId, const QString &methodName, const QString &methodBody);
    quint32 resetBindingForObject(int objectDebugId, const QString &propertyName);
    quint32 queryExpressionResult(int objectDebugId, const QString &expr);
    void clearComponentCache();

    bool addObjectWatch(int objectDebugId);
    bool isObjectBeingWatched(int objectDebugId);
    bool removeObjectWatch(int objectDebugId);
    void removeAllObjectWatches();

    // returns the object references
    QList<QmlDebugObjectReference> objectReferences() const;
    QmlDebugObjectReference objectReferenceForId(int debugId) const;
    QmlDebugObjectReference objectReferenceForId(const QString &objectId) const;
    QmlDebugObjectReference objectReferenceForLocation(const int line, const int column) const;
    QList<QmlDebugObjectReference> rootObjectReference() const;
    DebugIdHash debugIdHash() const { return m_debugIdHash; }

    bool isConnected() const;

    void setSelectedItemsByDebugId(const QList<int> &debugIds);
    void setSelectedItemsByObjectId(const QList<QmlDebugObjectReference> &objectRefs);

    QList<QmlDebugEngineReference> engines() const;

    Debugger::QmlAdapter *qmlAdapter() const;

    quint32 fetchContextObject(const QmlDebugObjectReference& obj);
    void addObjectToTree(const QmlDebugObjectReference &obj);
    void fetchContextObjectRecursive(const QmlDebugContextReference &context, bool clear);
    void insertObjectInTreeIfNeeded(const QmlDebugObjectReference &object);

signals:
    void objectTreeUpdated();
    void connectionStatusMessage(const QString &text);

    void aboutToReloadEngines();
    void enginesChanged();

    void selectedItemsChanged(const QList<QmlDebugObjectReference> &selectedItems);

    void connected();
    void disconnected();

    void selectToolActivated();
    void selectMarqueeToolActivated();
    void zoomToolActivated();
    void animationSpeedChanged(qreal slowDownFactor);
    void animationPausedChanged(bool paused);
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void serverReloaded();
    void propertyChanged(int debugId, const QByteArray &propertyName, const QVariant &propertyValue);

    void result(quint32 queryId, const QVariant &result);
    void rootContext(const QVariant &context);

public slots:
    void refreshObjectTree();
    void queryEngineContext(int id);
    void reloadQmlViewer();

    void setDesignModeBehavior(bool inDesignMode);
    void setAnimationSpeed(qreal slowDownFactor);
    void setAnimationPaused(bool paused);
    void changeToZoomTool();
    void changeToSelectTool();
    void changeToSelectMarqueeTool();
    void showAppOnTop(bool showOnTop);
    void createQmlObject(const QString &qmlText, int parentDebugId,
                         const QStringList &imports, const QString &filename = QString(), int order = 0);
    void destroyQmlObject(int debugId);
    void reparentQmlObject(int debugId, int newParent);

private slots:
    void connectToServer();
    void clientStatusChanged(QDeclarativeDebugClient::Status status);
    void engineClientStatusChanged(QDeclarativeDebugClient::Status status);

    void onCurrentObjectsChanged(const QList<int> &debugIds, bool requestIfNeeded = true);
    void newObjects();
    void objectWatchTriggered(int debugId, const QByteArray &propertyName, const QVariant &propertyValue);
    void onResult(quint32 queryId, const QVariant &value, const QByteArray &type);
    void onCurrentObjectsFetched(quint32 queryId, const QVariant &result);

private:
    void contextChanged(const QVariant &value);
    void updateEngineList(const QVariant &value);
    void objectTreeFetched(quint32 queryId, const QVariant &result);
    void updateConnected();
    void reloadEngines();

    QList<QmlDebugObjectReference> objectReferences(const QmlDebugObjectReference &objectRef) const;
    QmlDebugObjectReference objectReferenceForId(int debugId, const QmlDebugObjectReference &ref) const;

    enum LogDirection {
        LogSend,
        LogReceive
    };
    void log(LogDirection direction, const QString &message);


private:
    void buildDebugIdHashRecursive(const QmlDebugObjectReference &ref);

    QWeakPointer<Debugger::QmlAdapter> m_adapter;
    QmlEngineDebugClient *m_engineClient;
    QmlJSInspectorClient *m_inspectorClient;

    quint32 m_engineQueryId;
    quint32 m_contextQueryId;
    QList<quint32> m_objectTreeQueryIds;
    QList<quint32> m_fetchCurrentObjectsQueryIds;

    QList<QmlDebugObjectReference> m_rootObjects;
    QList<QmlDebugObjectReference> m_fetchCurrentObjects;
    QmlDebugEngineReferenceList m_engines;
    DebugIdHash m_debugIdHash;

    QList<int> m_objectWatches;

    bool m_isConnected;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSCLIENTPROXY_H
