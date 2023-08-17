// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#pragma once

#include "tasking_global.h"

#include "tasktree.h"

#include <QNetworkReply>
#include <QNetworkRequest>

#include <memory>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
QT_END_NAMESPACE

namespace Tasking {

// This class introduces the dependency to Qt::Network, otherwise Tasking namespace
// is independent on Qt::Network.
// Possibly, it could be placed inside Qt::Network library, as a wrapper around QNetworkReply.

class TASKING_EXPORT NetworkQuery final : public QObject
{
    Q_OBJECT

public:
    ~NetworkQuery();
    void setRequest(const QNetworkRequest &request) { m_request = request; }
    void setNetworkAccessManager(QNetworkAccessManager *manager) { m_manager = manager; }
    QNetworkReply *reply() const { return m_reply.get(); }
    void start();

signals:
    void started();
    void done(bool success);

private:
    QNetworkRequest m_request;
    QNetworkAccessManager *m_manager = nullptr;
    std::unique_ptr<QNetworkReply> m_reply;
};

class TASKING_EXPORT NetworkQueryTaskAdapter : public TaskAdapter<NetworkQuery>
{
public:
    NetworkQueryTaskAdapter() { connect(task(), &NetworkQuery::done, this, &TaskInterface::done); }
    void start() final { task()->start(); }
};

using NetworkQueryTask = CustomTask<NetworkQueryTaskAdapter>;

} // namespace Tasking
