#ifndef QMLJSCLIENTPROXY_H
#define QMLJSCLIENTPROXY_H

#include "qmljsinspectorplugin.h"

#include <private/qdeclarativedebug_p.h>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QDeclarativeEngineDebug)
QT_FORWARD_DECLARE_CLASS(QDeclarativeDebugConnection)
QT_FORWARD_DECLARE_CLASS(QDeclarativeDebugExpressionQuery)
QT_FORWARD_DECLARE_CLASS(QDeclarativeDebugPropertyDump)

namespace Debugger {
    class DebuggerRunControl;
}

namespace QmlJSInspector {
namespace Internal {

class QmlInspectorPlugin;

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

    void propertyDumpReceived(const QDeclarativeDebugPropertyDump &propertyDump);
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
    friend class QmlJSInspector::Internal::QmlInspectorPlugin;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSCLIENTPROXY_H
