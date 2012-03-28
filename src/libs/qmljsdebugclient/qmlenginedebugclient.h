/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QMLENGINEDEBUGCLIENT_H
#define QMLENGINEDEBUGCLIENT_H

#include "qmljsdebugclient_global.h"
#include "qdeclarativedebugclient.h"
#include <qurl.h>
#include <qvariant.h>

namespace QmlJsDebugClient {

class QDeclarativeDebugConnection;
class QmlDebugPropertyReference;
class QmlDebugContextReference;
class QmlDebugObjectReference;
class QmlDebugFileReference;
class QmlDebugEngineReference;

class QMLJSDEBUGCLIENT_EXPORT QmlEngineDebugClient : public QDeclarativeDebugClient
{
    Q_OBJECT
public:
    QmlEngineDebugClient(const QString &clientName,
                         QDeclarativeDebugConnection *conn);

    quint32 addWatch(const QmlDebugPropertyReference &property);
    quint32 addWatch(const QmlDebugContextReference &context, const QString &id);
    quint32 addWatch(const QmlDebugObjectReference &object, const QString &expr);
    quint32 addWatch(const QmlDebugObjectReference &object);
    quint32 addWatch(const QmlDebugFileReference &file);

    void removeWatch(quint32 watch);

    quint32 queryAvailableEngines();
    quint32 queryRootContexts(const QmlDebugEngineReference &context);
    quint32 queryObject(const QmlDebugObjectReference &object);
    quint32 queryObjectRecursive(const QmlDebugObjectReference &object);
    quint32 queryExpressionResult(int objectDebugId,
                                  const QString &expr);
    virtual quint32 setBindingForObject(int objectDebugId, const QString &propertyName,
                                const QVariant &bindingExpression,
                                bool isLiteralValue,
                                QString source, int line);
    virtual quint32 resetBindingForObject(int objectDebugId,
                                  const QString &propertyName);
    virtual quint32 setMethodBody(int objectDebugId, const QString &methodName,
                          const QString &methodBody);

signals:
    void newStatus(QDeclarativeDebugClient::Status status);
    void newObjects();
    void valueChanged(int debugId, const QByteArray &name,
                      const QVariant &value);
    void result(quint32 queryId, const QVariant &result, const QByteArray &type);

protected:
    virtual void statusChanged(Status status);
    virtual void messageReceived(const QByteArray &);

    quint32 getId() { return m_nextId++; }

    void decode(QDataStream &d, QmlDebugContextReference &context);
    void decode(QDataStream &d, QmlDebugObjectReference &object, bool simple);

private:
    quint32 m_nextId;
};

class QmlDebugFileReference
{
public:
    QmlDebugFileReference() : m_lineNumber(-1), m_columnNumber(-1) {}

    QUrl url() const { return m_url; }
    int lineNumber() const { return m_lineNumber; }
    int columnNumber() const { return m_columnNumber; }

private:
    friend class QmlEngineDebugClient;
    QUrl m_url;
    int m_lineNumber;
    int m_columnNumber;
};

class QmlDebugEngineReference
{
public:
    QmlDebugEngineReference() : m_debugId(-1) {}
    QmlDebugEngineReference(int id) : m_debugId(id) {}

    int debugId() const { return m_debugId; }
    QString name() const { return m_name; }

private:
    friend class QmlEngineDebugClient;
    int m_debugId;
    QString m_name;
};

typedef QList<QmlDebugEngineReference> QmlDebugEngineReferenceList;

class QmlDebugObjectReference
{
public:
    QmlDebugObjectReference() : m_debugId(-1), m_parentId(-1), m_contextDebugId(-1), m_needsMoreData(false) {}
    QmlDebugObjectReference(int id) : m_debugId(id), m_parentId(-1), m_contextDebugId(-1), m_needsMoreData(false) {}

    int debugId() const { return m_debugId; }
    int parentId() const { return m_parentId; }
    QString className() const { return m_className; }
    QString idString() const { return m_idString; }
    QString name() const { return m_name; }

    QmlDebugFileReference source() const { return m_source; }
    int contextDebugId() const { return m_contextDebugId; }
    bool needsMoreData() const { return m_needsMoreData; }

    QList<QmlDebugPropertyReference> properties() const { return m_properties; }
    QList<QmlDebugObjectReference> children() const { return m_children; }

    bool insertObjectInTree(const QmlDebugObjectReference &obj)
    {
        for (int i = 0; i < m_children.count(); i++) {
            if (m_children[i].debugId() == obj.debugId()) {
                m_children.replace(i, obj);
                return true;
            } else {
                if (m_children[i].insertObjectInTree(obj))
                    return true;
            }
        }
        return false;
    }

    bool operator ==(const QmlDebugObjectReference &obj)
    {
        return m_debugId == obj.debugId();
    }

private:
    friend class QmlEngineDebugClient;
    int m_debugId;
    int m_parentId;
    QString m_className;
    QString m_idString;
    QString m_name;
    QmlDebugFileReference m_source;
    int m_contextDebugId;
    bool m_needsMoreData;
    QList<QmlDebugPropertyReference> m_properties;
    QList<QmlDebugObjectReference> m_children;
};

class QmlDebugContextReference
{
public:
    QmlDebugContextReference() : m_debugId(-1) {}

    int debugId() const { return m_debugId; }
    QString name() const { return m_name; }

    QList<QmlDebugObjectReference> objects() const { return m_objects; }
    QList<QmlDebugContextReference> contexts() const { return m_contexts; }

private:
    friend class QmlEngineDebugClient;
    int m_debugId;
    QString m_name;
    QList<QmlDebugObjectReference> m_objects;
    QList<QmlDebugContextReference> m_contexts;
};

class QmlDebugPropertyReference
{
public:
    QmlDebugPropertyReference() : m_objectDebugId(-1), m_hasNotifySignal(false) {}

    int debugId() const { return m_objectDebugId; }
    QString name() const { return m_name; }
    QVariant value() const { return m_value; }
    QString valueTypeName() const { return m_valueTypeName; }
    QString binding() const { return m_binding; }
    bool hasNotifySignal() const { return m_hasNotifySignal; }

private:
    friend class QmlEngineDebugClient;
    int m_objectDebugId;
    QString m_name;
    QVariant m_value;
    QString m_valueTypeName;
    QString m_binding;
    bool m_hasNotifySignal;
};

} // namespace QmlJsDebugClient

Q_DECLARE_METATYPE(QmlJsDebugClient::QmlDebugObjectReference)
Q_DECLARE_METATYPE(QmlJsDebugClient::QmlDebugEngineReference)
Q_DECLARE_METATYPE(QmlJsDebugClient::QmlDebugEngineReferenceList)
Q_DECLARE_METATYPE(QmlJsDebugClient::QmlDebugContextReference)

#endif // QMLENGINEDEBUGCLIENT_H
