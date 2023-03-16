// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "qmldesignercorelib_global.h"

#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QQuickView;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractView;
class BakeLightsConnectionManager;
class NodeInstanceView;
class RewriterView;

class BakeLights : public QObject
{
    Q_OBJECT

public:
    BakeLights(AbstractView *view);
    ~BakeLights();

    Q_INVOKABLE void cancel();

    void raiseDialog();

    static QString resolveView3dId(AbstractView *view);

signals:
    void finished();
    void progress(const QString &msg);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void bakeLights();

    QPointer<QQuickView> m_dialog;
    QPointer<BakeLightsConnectionManager> m_connectionManager;
    QPointer<NodeInstanceView> m_nodeInstanceView;
    QPointer<RewriterView> m_rewriterView;
    QPointer<AbstractView> m_view;
    ModelPointer m_model;
    QString m_view3dId;
};

} // namespace QmlDesigner
