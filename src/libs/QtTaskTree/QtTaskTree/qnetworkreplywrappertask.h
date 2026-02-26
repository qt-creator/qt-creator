// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QTASKTREE_QNETWORKREPLYWRAPPERTASK_H
#define QTASKTREE_QNETWORKREPLYWRAPPERTASK_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtNetwork/QNetworkReply>

QT_BEGIN_NAMESPACE

class QNetworkAccessManager;
class QNetworkRequest;

namespace QtTaskTree {

class QNetworkReplyWrapperPrivate;

class Q_TASKTREE_EXPORT QNetworkReplyWrapper : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QNetworkReplyWrapper)

public:
    QNetworkReplyWrapper() : QNetworkReplyWrapper(nullptr) {}
    explicit QNetworkReplyWrapper(QObject *parent);
    ~QNetworkReplyWrapper() override;
    void setRequest(const QNetworkRequest &request);
    void setOperation(QNetworkAccessManager::Operation operation);
    void setVerb(const QByteArray &verb);
    void setData(const QByteArray &data);
    void setNetworkAccessManager(QNetworkAccessManager *manager);
    QNetworkReply *reply() const;
    void start();

Q_SIGNALS:
    void started();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
#if QT_CONFIG(ssl)
    void sslErrors(const QList<QSslError> &errors);
#endif
    void done(QtTaskTree::DoneResult result);

protected:
    bool event(QEvent *event) override;
};

using QNetworkReplyWrapperTask = QCustomTask<QNetworkReplyWrapper>;

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QTASKTREE_QNETWORKREPLYWRAPPERTASK_H
