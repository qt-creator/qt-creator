// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "modelnode.h"
#include "qmldesignercorelib_global.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE
class QQuickView;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractView;
class BakeLightsConnectionManager;
class NodeInstanceView;
class RewriterView;
class BakeLightsDataModel;

class BakeLights : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool manualMode READ manualMode WRITE setManualMode NOTIFY manualModeChanged)

public:
    BakeLights(AbstractView *view, ModulesStorage &modulesStorage);
    ~BakeLights();

    Q_INVOKABLE void cancel();
    Q_INVOKABLE void bakeLights();
    Q_INVOKABLE void apply();
    Q_INVOKABLE void rebake();
    Q_INVOKABLE void exposeModelsAndLights(const QString &nodeId);

    void raiseDialog();

    bool manualMode() const;
    void setManualMode(bool enabled);
    void setKitVersion(const QVersionNumber &version);

signals:
    void finished();
    void progressChanged(double prog);
    void timeRemainingChanged(double seconds);
    void message(const QString &msg);
    void manualModeChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void showSetupDialog();
    void showProgressDialog();
    void cleanup();
    void handlePendingRebakeTimeout();
    void pendingRebakeCleanup();

    // Separate dialogs for setup and progress, as setup needs to be modal
    QPointer<QQuickView> m_setupDialog;
    QPointer<QQuickView> m_progressDialog;

    QPointer<BakeLightsConnectionManager> m_connectionManager;
    QPointer<NodeInstanceView> m_nodeInstanceView;
    QPointer<RewriterView> m_rewriterView;
    QPointer<AbstractView> m_view;
    ModulesStorage &m_modulesStorage;
    QPointer<BakeLightsDataModel> m_dataModel;
    ModelPointer m_model;
    QString m_view3dId;
    bool m_manualMode = false;
    QVersionNumber m_kitVersion;
    QTimer m_pendingRebakeTimer;
    ModelNode m_pendingRebakeCheckNode;
    int m_pendingRebakeTimerCount = 0;
};

} // namespace QmlDesigner
