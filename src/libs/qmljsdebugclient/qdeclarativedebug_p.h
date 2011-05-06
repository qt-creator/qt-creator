/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef QDECLARATIVEDEBUG_H
#define QDECLARATIVEDEBUG_H

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

QT_BEGIN_HEADER

namespace QmlJsDebugClient {

class QDeclarativeDebugConnection;
class QDeclarativeDebugWatch;
class QDeclarativeDebugPropertyWatch;
class QDeclarativeDebugObjectExpressionWatch;
class QDeclarativeDebugEnginesQuery;
class QDeclarativeDebugRootContextQuery;
class QDeclarativeDebugObjectQuery;
class QDeclarativeDebugExpressionQuery;
class QDeclarativeDebugPropertyReference;
class QDeclarativeDebugContextReference;
class QDeclarativeDebugObjectReference;
class QDeclarativeDebugFileReference;
class QDeclarativeDebugEngineReference;
class QDeclarativeEngineDebugPrivate;

class QDeclarativeEngineDebug : public QObject
{
Q_OBJECT
public:
    enum Status { NotConnected, Unavailable, Enabled };

    explicit QDeclarativeEngineDebug(QDeclarativeDebugConnection *, QObject * = 0);
    ~QDeclarativeEngineDebug();

    Status status() const;

    QDeclarativeDebugPropertyWatch *addWatch(const QDeclarativeDebugPropertyReference &,
                            QObject *parent = 0);
    QDeclarativeDebugWatch *addWatch(const QDeclarativeDebugContextReference &, const QString &,
                            QObject *parent = 0);
    QDeclarativeDebugObjectExpressionWatch *addWatch(const QDeclarativeDebugObjectReference &, const QString &,
                            QObject *parent = 0);
    QDeclarativeDebugWatch *addWatch(const QDeclarativeDebugObjectReference &,
                            QObject *parent = 0);
    QDeclarativeDebugWatch *addWatch(const QDeclarativeDebugFileReference &,
                            QObject *parent = 0);

    void removeWatch(QDeclarativeDebugWatch *watch);

    QDeclarativeDebugEnginesQuery *queryAvailableEngines(QObject *parent = 0);
    QDeclarativeDebugRootContextQuery *queryRootContexts(const QDeclarativeDebugEngineReference &,
                                                QObject *parent = 0);
    QDeclarativeDebugObjectQuery *queryObject(const QDeclarativeDebugObjectReference &, 
                                     QObject *parent = 0);
    QDeclarativeDebugObjectQuery *queryObjectRecursive(const QDeclarativeDebugObjectReference &, 
                                              QObject *parent = 0);
    QDeclarativeDebugExpressionQuery *queryExpressionResult(int objectDebugId, 
                                                   const QString &expr,
                                                   QObject *parent = 0);
    bool setBindingForObject(int objectDebugId, const QString &propertyName,
                             const QVariant &bindingExpression, bool isLiteralValue,
                             QString source = QString(), int line = -1);
    bool resetBindingForObject(int objectDebugId, const QString &propertyName);
    bool setMethodBody(int objectDebugId, const QString &methodName, const QString &methodBody);

Q_SIGNALS:
    void newObjects();
    void statusChanged(Status status);

private:
    Q_DECLARE_PRIVATE(QDeclarativeEngineDebug)
    QScopedPointer<QDeclarativeEngineDebugPrivate> d_ptr;
};

class QDeclarativeDebugWatch : public QObject
{
Q_OBJECT
public:
    enum State { Waiting, Active, Inactive, Dead };

    QDeclarativeDebugWatch(QObject *);
    ~QDeclarativeDebugWatch();

    int queryId() const;
    int objectDebugId() const;
    State state() const;

Q_SIGNALS:
    void stateChanged(QDeclarativeDebugWatch::State);
    //void objectChanged(int, const QDeclarativeDebugObjectReference &);
    //void valueChanged(int, const QVariant &);

    // Server sends value as string if it is a user-type variant
    void valueChanged(const QByteArray &name, const QVariant &value);

private:
    friend class QDeclarativeEngineDebug;
    friend class QDeclarativeEngineDebugPrivate;
    void setState(State);
    State m_state;
    int m_queryId;
    QDeclarativeEngineDebug *m_client;
    int m_objectDebugId;
};

class QDeclarativeDebugPropertyWatch : public QDeclarativeDebugWatch
{
    Q_OBJECT
public:
    QDeclarativeDebugPropertyWatch(QObject *parent);

    QString name() const;

private:
    friend class QDeclarativeEngineDebug;
    QString m_name;
};

class QDeclarativeDebugObjectExpressionWatch : public QDeclarativeDebugWatch
{
    Q_OBJECT
public:
    QDeclarativeDebugObjectExpressionWatch(QObject *parent);

    QString expression() const;

private:
    friend class QDeclarativeEngineDebug;
    QString m_expr;
    int m_debugId;
};

class QDeclarativeDebugQuery : public QObject
{
Q_OBJECT
public:
    enum State { Waiting, Error, Completed };

    State state() const;
    bool isWaiting() const;

//    bool waitUntilCompleted();

Q_SIGNALS:
    void stateChanged(QDeclarativeDebugQuery::State);

protected:
    QDeclarativeDebugQuery(QObject *);

private:
    friend class QDeclarativeEngineDebug;
    friend class QDeclarativeEngineDebugPrivate;
    void setState(State);
    State m_state;
};

class QDeclarativeDebugFileReference
{
public:
    QDeclarativeDebugFileReference();
    QDeclarativeDebugFileReference(const QDeclarativeDebugFileReference &);
    QDeclarativeDebugFileReference &operator=(const QDeclarativeDebugFileReference &);

    QUrl url() const;
    void setUrl(const QUrl &);
    int lineNumber() const;
    void setLineNumber(int);
    int columnNumber() const;
    void setColumnNumber(int);

private:
    friend class QDeclarativeEngineDebugPrivate;
    QUrl m_url;
    int m_lineNumber;
    int m_columnNumber;
};

class QDeclarativeDebugEngineReference
{
public:
    QDeclarativeDebugEngineReference();
    QDeclarativeDebugEngineReference(int);
    QDeclarativeDebugEngineReference(const QDeclarativeDebugEngineReference &);
    QDeclarativeDebugEngineReference &operator=(const QDeclarativeDebugEngineReference &);

    int debugId() const;
    QString name() const;

private:
    friend class QDeclarativeEngineDebugPrivate;
    int m_debugId;
    QString m_name;
};

class QDeclarativeDebugObjectReference
{
public:
    QDeclarativeDebugObjectReference();
    QDeclarativeDebugObjectReference(int);
    QDeclarativeDebugObjectReference(const QDeclarativeDebugObjectReference &);
    QDeclarativeDebugObjectReference &operator=(const QDeclarativeDebugObjectReference &);

    int debugId() const;
    QString className() const;
    QString idString() const;
    QString name() const;

    QDeclarativeDebugFileReference source() const;
    int contextDebugId() const;

    QList<QDeclarativeDebugPropertyReference> properties() const;
    QList<QDeclarativeDebugObjectReference> children() const;

private:
    friend class QDeclarativeEngineDebugPrivate;
    int m_debugId;
    QString m_class;
    QString m_idString;
    QString m_name;
    QDeclarativeDebugFileReference m_source;
    int m_contextDebugId;
    QList<QDeclarativeDebugPropertyReference> m_properties;
    QList<QDeclarativeDebugObjectReference> m_children;
};

class QDeclarativeDebugContextReference
{
public:
    QDeclarativeDebugContextReference();
    QDeclarativeDebugContextReference(const QDeclarativeDebugContextReference &);
    QDeclarativeDebugContextReference &operator=(const QDeclarativeDebugContextReference &);

    int debugId() const;
    QString name() const;

    QList<QDeclarativeDebugObjectReference> objects() const;
    QList<QDeclarativeDebugContextReference> contexts() const;

private:
    friend class QDeclarativeEngineDebugPrivate;
    int m_debugId;
    QString m_name;
    QList<QDeclarativeDebugObjectReference> m_objects;
    QList<QDeclarativeDebugContextReference> m_contexts;
};

class QDeclarativeDebugPropertyReference
{
public:
    QDeclarativeDebugPropertyReference();
    QDeclarativeDebugPropertyReference(const QDeclarativeDebugPropertyReference &);
    QDeclarativeDebugPropertyReference &operator=(const QDeclarativeDebugPropertyReference &);

    int objectDebugId() const;
    QString name() const;
    QVariant value() const;
    QString valueTypeName() const;
    QString binding() const;
    bool hasNotifySignal() const;

private:
    friend class QDeclarativeEngineDebugPrivate;
    int m_objectDebugId;
    QString m_name;
    QVariant m_value;
    QString m_valueTypeName;
    QString m_binding;
    bool m_hasNotifySignal;
};


class QDeclarativeDebugEnginesQuery : public QDeclarativeDebugQuery
{
Q_OBJECT
public:
    virtual ~QDeclarativeDebugEnginesQuery();
    QList<QDeclarativeDebugEngineReference> engines() const;
private:
    friend class QDeclarativeEngineDebug;
    friend class QDeclarativeEngineDebugPrivate;
    QDeclarativeDebugEnginesQuery(QObject *);
    QDeclarativeEngineDebug *m_client;
    int m_queryId;
    QList<QDeclarativeDebugEngineReference> m_engines;
};

class QDeclarativeDebugRootContextQuery : public QDeclarativeDebugQuery
{
Q_OBJECT
public:
    virtual ~QDeclarativeDebugRootContextQuery();
    QDeclarativeDebugContextReference rootContext() const;
private:
    friend class QDeclarativeEngineDebug;
    friend class QDeclarativeEngineDebugPrivate;
    QDeclarativeDebugRootContextQuery(QObject *);
    QDeclarativeEngineDebug *m_client;
    int m_queryId;
    QDeclarativeDebugContextReference m_context;
};

class QDeclarativeDebugObjectQuery : public QDeclarativeDebugQuery
{
Q_OBJECT
public:
    virtual ~QDeclarativeDebugObjectQuery();
    QDeclarativeDebugObjectReference object() const;
private:
    friend class QDeclarativeEngineDebug;
    friend class QDeclarativeEngineDebugPrivate;
    QDeclarativeDebugObjectQuery(QObject *);
    QDeclarativeEngineDebug *m_client;
    int m_queryId;
    QDeclarativeDebugObjectReference m_object;

};

class QDeclarativeDebugExpressionQuery : public QDeclarativeDebugQuery
{
Q_OBJECT
public:
    virtual ~QDeclarativeDebugExpressionQuery();
    QVariant expression() const;
    QVariant result() const;
private:
    friend class QDeclarativeEngineDebug;
    friend class QDeclarativeEngineDebugPrivate;
    QDeclarativeDebugExpressionQuery(QObject *);
    QDeclarativeEngineDebug *m_client;
    int m_queryId;
    QVariant m_expr;
    QVariant m_result;
};

}

Q_DECLARE_METATYPE(QmlJsDebugClient::QDeclarativeDebugEngineReference)
Q_DECLARE_METATYPE(QmlJsDebugClient::QDeclarativeDebugObjectReference)
Q_DECLARE_METATYPE(QmlJsDebugClient::QDeclarativeDebugContextReference)
Q_DECLARE_METATYPE(QmlJsDebugClient::QDeclarativeDebugPropertyReference)


#endif // QDECLARATIVEDEBUG_H
