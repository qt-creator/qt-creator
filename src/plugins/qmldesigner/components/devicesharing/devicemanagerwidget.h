// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <studioquickwidget.h>

#include <QElapsedTimer>
#include <QPointer>
#include <QQmlPropertyMap>

#include "devicemanager.h"
#include "devicemanagermodel.h"

QT_BEGIN_NAMESPACE
class QShortcut;
QT_END_NAMESPACE

namespace QmlDesigner::DeviceShare {

class DeviceManagerWidget : public StudioQuickWidget
{
    Q_OBJECT

public:
    DeviceManagerWidget(DeviceManager &deviceManager, QWidget *parent = nullptr);
    ~DeviceManagerWidget() override;

    static QString qmlSourcesPath();

protected:
    void showEvent(QShowEvent *) override;
    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    void reloadQmlSource();

private:
    QShortcut *m_qmlSourceUpdateShortcut;
    QElapsedTimer m_usageTimer;

    DeviceManagerModel m_deviceManagerModel;
};

} // namespace QmlDesigner::DeviceShare
