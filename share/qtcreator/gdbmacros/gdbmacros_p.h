/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef GDBMACROS_P_H
#define GDBMACROS_P_H

#include <QtCore/QObject>
#include <QtCore/QPointer>

#ifndef QT_BOOTSTRAPPED

#undef NS
#ifdef QT_NAMESPACE
#   define STRINGIFY0(s) #s
#   define STRINGIFY1(s) STRINGIFY0(s)
#   define NS STRINGIFY1(QT_NAMESPACE) "::"
#   define NSX "'" STRINGIFY1(QT_NAMESPACE) "::"
#   define NSY "'"
#else
#   define NS ""
#   define NSX "'"
#   define NSY "'"
#endif

#if defined(QT_BEGIN_NAMESPACE)
QT_BEGIN_NAMESPACE
#endif

struct Sender { QObject *sender; int signal; int ref; };

#if QT_VERSION < 0x040600
struct Connection
{
    QObject *receiver;
    int method;
    uint connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
    QBasicAtomicPointer<int> argumentTypes;
};

typedef QList<Connection> ConnectionList;
typedef QList<Sender> SenderList;

static inline const Connection &connectionAt(const ConnectionList &l, int i) { return l.at(i); }
static inline const QObject *senderAt(const SenderList &l, int i) { return l.at(i).sender; }
static inline int signalAt(const SenderList &l, int i) { return l.at(i).signal; }
#else
struct Connection
{
    QObject *sender;
    QObject *receiver;
    int method;
    uint connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
    QBasicAtomicPointer<int> argumentTypes;
    Connection *nextConnectionList;
    //senders linked list
    Connection *next;
    Connection **prev;
};

struct ConnectionList
{
    ConnectionList() : first(0), last(0) { }
    int size() const
    {
        int count = 0;
        for (Connection *c = first; c != 0; c = c->nextConnectionList)
            ++count;
        return count;
    }

    Connection *first;
    Connection *last;
};

typedef Connection *SenderList;

static inline const Connection &connectionAt(const ConnectionList &l, int i)
{
    Connection *conn = l.first;
    for (int cnt = 0; cnt < i; ++cnt)
        conn = conn->nextConnectionList;
    return *conn;
}
#endif

class ObjectPrivate : public QObjectData
{
public:
    ObjectPrivate() {}
    virtual ~ObjectPrivate() {}

#if QT_VERSION < 0x040600
    QList<QObject *> pendingChildInsertedEvents;
    void *threadData;
    void *currentSender;
    void *currentChildBeingDeleted;
    QList<QPointer<QObject> > eventFilters;

    void *extraData;
    mutable quint32 connectedSignals;
    QString objectName;

    void *connectionLists;
    SenderList senders;
    int *deleteWatch;
#else
    QString objectName;
    void *extraData;
    void *threadData;
    void *connectionLists;
    SenderList senders;
    void *currentSender;
    mutable quint32 connectedSignals[2];
    QList<QObject *> pendingChildInsertedEvents;
    QList<QPointer<QObject> > eventFilters;
    void *currentChildBeingDeleted;
    QAtomicPointer<void> sharedRefcount;
    int *deleteWatch;
#endif
};

#endif // QT_BOOTSTRAPPED

#if defined(QT_BEGIN_NAMESPACE)
QT_END_NAMESPACE
#endif

#endif // GDBMACROS_P_H
