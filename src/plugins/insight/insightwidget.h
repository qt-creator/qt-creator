// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <QQmlPropertyMap>
#include <QQuickWidget>

QT_BEGIN_NAMESPACE
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceView;
class InsightModel;
class InsightView;

class InsightWidget : public QQuickWidget
{
    Q_OBJECT

public:
    InsightWidget(InsightView *insightView, InsightModel *insightModel);
    ~InsightWidget() override;

    static QString qmlSourcesPath();

protected:
    void showEvent(QShowEvent *) override;
    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    void reloadQmlSource();

private:
    QPointer<InsightView> m_insightView;
    QShortcut *m_qmlSourceUpdateShortcut;
    QElapsedTimer m_usageTimer;
};

} // namespace QmlDesigner
