/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#ifndef QMLJSCLIENTPROXY_H
#define QMLJSCLIENTPROXY_H

#include "qmljsinspectorplugin.h"
#include "qmljsprivateapi.h"
#include <QtCore/QObject>

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

    bool setBindingForObject(int objectDebugId,
                             const QString &propertyName,
                             const QVariant &value,
                             bool isLiteralValue,
                             QString source,
                             int line);

    bool setMethodBodyForObject(int objectDebugId, const QString &methodName, const QString &methodBody);
    bool resetBindingForObject(int objectDebugId, const QString &propertyName);
    QDeclarativeDebugExpressionQuery *queryExpressionResult(int objectDebugId, const QString &expr);
    void clearComponentCache();

    bool addObjectWatch(int objectDebugId);
    bool removeObjectWatch(int objectDebugId);
    void removeAllObjectWatches();

    // returns the object references
    QList<QDeclarativeDebugObjectReference> objectReferences() const;
    QDeclarativeDebugObjectReference objectReferenceForId(int debugId) const;
    QDeclarativeDebugObjectReference objectReferenceForId(const QString &objectId) const;
    QDeclarativeDebugObjectReference objectReferenceForLocation(const int line, const int column) const;
    QList<QDeclarativeDebugObjectReference> rootObjectReference() const;
    DebugIdHash debugIdHash() const { return m_debugIdHash; }

    bool isConnected() const;

    void setSelectedItemsByDebugId(const QList<int> &debugIds);
    void setSelectedItemsByObjectId(const QList<QDeclarativeDebugObjectReference> &objectRefs);

    QList<QDeclarativeDebugEngineReference> engines() const;

    Debugger::QmlAdapter *qmlAdapter() const;

signals:
    void objectTreeUpdated();
    void connectionStatusMessage(const QString &text);

    void aboutToReloadEngines();
    void enginesChanged();

    void selectedItemsChanged(const QList<QDeclarativeDebugObjectReference> &selectedItems);

    void connected();
    void disconnected();

    void colorPickerActivated();
    void selectToolActivated();
    void selectMarqueeToolActivated();
    void zoomToolActivated();
    void animationSpeedChanged(qreal slowDownFactor);
    void animationPausedChanged(bool paused);
    void designModeBehaviorChanged(bool inDesignMode);
    void showAppOnTopChanged(bool showAppOnTop);
    void serverReloaded();
    void selectedColorChanged(const QColor &color);
    void propertyChanged(int debugId, const QByteArray &propertyName, const QVariant &propertyValue);

public slots:
    void refreshObjectTree();
    void queryEngineContext(int id);
    void reloadQmlViewer();

    void setDesignModeBehavior(bool inDesignMode);
    void setAnimationSpeed(qreal slowDownFactor);
    void setAnimationPaused(bool paused);
    void changeToColorPickerTool();
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

    void contextChanged();

    void onCurrentObjectsChanged(const QList<int> &debugIds, bool requestIfNeeded = true);
    void updateEngineList();
    void objectTreeFetched(QDeclarativeDebugQuery::State state = QDeclarativeDebugQuery::Completed);
    void fetchContextObjectRecursive(const QmlJsDebugClient::QDeclarativeDebugContextReference& context);
    void newObjects();
    void objectWatchTriggered(const QByteArray &propertyName, const QVariant &propertyValue);

private:
    void updateConnected();
    void reloadEngines();

    QList<QDeclarativeDebugObjectReference> objectReferences(const QDeclarativeDebugObjectReference &objectRef) const;
    QDeclarativeDebugObjectReference objectReferenceForId(int debugId, const QDeclarativeDebugObjectReference &ref) const;

    enum LogDirection {
        LogSend,
        LogReceive
    };
    void log(LogDirection direction, const QString &message);


private:
    Q_DISABLE_COPY(ClientProxy)
    void buildDebugIdHashRecursive(const QDeclarativeDebugObjectReference &ref);

    QWeakPointer<Debugger::QmlAdapter> m_adapter;
    QDeclarativeEngineDebug *m_engineClient;
    QmlJSInspectorClient *m_inspectorClient;

    QDeclarativeDebugEnginesQuery *m_engineQuery;
    QDeclarativeDebugRootContextQuery *m_contextQuery;
    QList<QDeclarativeDebugObjectQuery *> m_objectTreeQuery;

    QList<QDeclarativeDebugObjectReference> m_rootObjects;
    QList<QDeclarativeDebugEngineReference> m_engines;
    QTimer m_requestObjectsTimer;
    DebugIdHash m_debugIdHash;

    QHash<int, QDeclarativeDebugWatch *> m_objectWatches;

    bool m_isConnected;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSCLIENTPROXY_H
