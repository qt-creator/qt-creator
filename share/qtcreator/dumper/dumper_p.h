/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GDBMACROS_P_H
#define GDBMACROS_P_H

#include <QObject>
#include <QPointer>

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
        int method_() const { return method; }
    };
#elif QT_VERSION < 0x040800
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
        int method_() const { return method; }
    };
#else
    typedef void (*StaticMetaCallFunction)(QObject *, QMetaObject::Call, int, void **);
    struct Connection
    {
        QObject *sender;
        QObject *receiver;
        StaticMetaCallFunction callFunction;
        // The next pointer for the singly-linked ConnectionList
        Connection *nextConnectionList;
        //senders linked list
        Connection *next;
        Connection **prev;
        QBasicAtomicPointer<int> argumentTypes;
        ushort method_offset;
        ushort method_relative;
        ushort connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
        ~Connection();
        int method() const { return method_offset + method_relative; }
        int method_() const { return method(); }
    };
#endif

#if QT_VERSION < 0x040600
    typedef QList<Connection> ConnectionList;
    typedef QList<Sender> SenderList;

    static inline const Connection &connectionAt(const ConnectionList &l, int i)
    {
        return l.at(i);
    }
    static inline const QObject *senderAt(const SenderList &l, int i)
    {
        return l.at(i).sender;
    }
    static inline int signalAt(const SenderList &l, int i)
    {
        return l.at(i).signal;
    }
//#elif QT_VERSION < 0x040800
#else
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
#elif QT_VERSION < 0x040800
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
#else
    QString objectName;
    void *extraData;
    void *threadData;
    void *connectionLists;
    Connection *senders;
    Sender *currentSender;
    mutable quint32 connectedSignals[2];
    void *unused;
    QList<QPointer<QObject> > eventFilters;
    union {
        QObject *currentChildBeingDeleted;
        void *declarativeData;
    };
    QAtomicPointer<void> sharedRefcount;
    #ifdef QT_JAMBI_BUILD
    int *deleteWatch;
    #endif
#endif
};

#endif // QT_BOOTSTRAPPED

#if defined(QT_BEGIN_NAMESPACE)
QT_END_NAMESPACE
#endif

#endif // GDBMACROS_P_H
