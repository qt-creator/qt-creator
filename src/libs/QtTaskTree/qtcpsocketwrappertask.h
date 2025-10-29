// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTCPSOCKETWRAPPERTASK_H
#define QTCPSOCKETWRAPPERTASK_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

QT_BEGIN_NAMESPACE

class QHostAddress;
class QTcpSocket;
class QTcpSocketWrapperPrivate;

class Q_TASKTREE_EXPORT QTcpSocketWrapper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QTcpSocketWrapper)

public:
    QTcpSocketWrapper(QObject *parent = nullptr);
    ~QTcpSocketWrapper() override;
    void setAddress(const QHostAddress &address);
    void setPort(quint16 port);
    void setData(const QByteArray &data);
    QTcpSocket *socket() const;
    void start();

Q_SIGNALS:
    void started();
    void done(QtTaskTree::DoneResult result);
};

using QTcpSocketWrapperTask = QCustomTask<QTcpSocketWrapper>;

QT_END_NAMESPACE

#endif // QTCPSOCKETWRAPPERTASK_H
