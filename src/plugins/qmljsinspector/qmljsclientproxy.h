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

namespace Debugger {
namespace Internal {
class QmlAdapter;
}
}

namespace QmlJSInspector {
namespace Internal {

class InspectorPlugin;
class QmlJSDesignDebugClient;

class ClientProxy : public QObject
{
    Q_OBJECT

public:
    explicit ClientProxy(Debugger::Internal::QmlAdapter *adapter, QObject *parent = 0);

    bool setBindingForObject(int objectDebugId,
                             const QString &propertyName,
                             const QVariant &value,
                             bool isLiteralValue);

    bool setMethodBodyForObject(int objectDebugId, const QString &methodName, const QString &methodBody);
    bool resetBindingForObject(int objectDebugId, const QString &propertyName);
    void clearComponentCache();

    // returns the object references for the given url.
    QList<QDeclarativeDebugObjectReference> objectReferences(const QUrl &url = QUrl()) const;
    QDeclarativeDebugObjectReference objectReferenceForId(int debugId) const;
    QDeclarativeDebugObjectReference rootObjectReference() const;

    bool isConnected() const;

    void setSelectedItemsByObjectId(const QList<QDeclarativeDebugObjectReference> &objectRefs);

    QList<QDeclarativeDebugEngineReference> engines() const;

    Debugger::Internal::QmlAdapter *qmlAdapter() const;

signals:
    void objectTreeUpdated(const QDeclarativeDebugObjectReference &rootObject);
    void connectionStatusMessage(const QString &text);

    void aboutToReloadEngines();
    void enginesChanged();

    void selectedItemsChanged(const QList<QDeclarativeDebugObjectReference> &selectedItems);

    void connected();
    void aboutToDisconnect();
    void disconnected();

    void colorPickerActivated();
    void selectToolActivated();
    void selectMarqueeToolActivated();
    void zoomToolActivated();
    void animationSpeedChanged(qreal slowdownFactor);
    void designModeBehaviorChanged(bool inDesignMode);
    void serverReloaded();
    void selectedColorChanged(const QColor &color);
    void contextPathUpdated(const QStringList &contextPath);

public slots:
    void refreshObjectTree();
    void queryEngineContext(int id);
    void reloadQmlViewer();

    void setDesignModeBehavior(bool inDesignMode);
    void setAnimationSpeed(qreal slowdownFactor = 1.0f);
    void changeToColorPickerTool();
    void changeToZoomTool();
    void changeToSelectTool();
    void changeToSelectMarqueeTool();
    void createQmlObject(const QString &qmlText, int parentDebugId,
                         const QStringList &imports, const QString &filename = QString());
    void destroyQmlObject(int debugId);
    void setContextPathIndex(int contextIndex);

private slots:
    void disconnectFromServer();
    void connectToServer();

    void contextChanged();

    void onCurrentObjectsChanged(const QList<int> &debugIds);
    void updateEngineList();
    void objectTreeFetched(QDeclarativeDebugQuery::State state = QDeclarativeDebugQuery::Completed);

private:
    bool isDesignClientConnected() const;
    void reloadEngines();

    QList<QDeclarativeDebugObjectReference> objectReferences(const QUrl &url, const QDeclarativeDebugObjectReference &objectRef) const;
    QDeclarativeDebugObjectReference objectReferenceForId(int debugId, const QDeclarativeDebugObjectReference &ref) const;

private:
    Q_DISABLE_COPY(ClientProxy);

    Debugger::Internal::QmlAdapter *m_adapter;
    QDeclarativeEngineDebug *m_client;
    QmlJSDesignDebugClient *m_designClient;

    QDeclarativeDebugEnginesQuery *m_engineQuery;
    QDeclarativeDebugRootContextQuery *m_contextQuery;
    QDeclarativeDebugObjectQuery *m_objectTreeQuery;

    QDeclarativeDebugObjectReference m_rootObject;
    QList<QDeclarativeDebugEngineReference> m_engines;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSCLIENTPROXY_H
