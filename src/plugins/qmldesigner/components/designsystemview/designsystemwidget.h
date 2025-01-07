// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <studioquickwidget.h>

#include <QElapsedTimer>
#include <QPointer>
#include <QQmlPropertyMap>

QT_BEGIN_NAMESPACE
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceView;

class AbstractView;
class DesignSystemInterface;
class DesignSystemView;

class DesignSystemWidget : public StudioQuickWidget
{
    Q_OBJECT

public:
    DesignSystemWidget(DesignSystemView *view, DesignSystemInterface *interface);
    ~DesignSystemWidget() override;

    static QString qmlSourcesPath();

protected:
    void showEvent(QShowEvent *) override;
    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    void reloadQmlSource();

private:
    QPointer<DesignSystemView> m_designSystemView;
    QShortcut *m_qmlSourceUpdateShortcut;
    QElapsedTimer m_usageTimer;
};

} // namespace QmlDesigner
