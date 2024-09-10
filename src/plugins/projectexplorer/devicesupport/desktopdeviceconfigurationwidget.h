// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicewidget.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer {

class DesktopDeviceConfigurationWidget : public IDeviceWidget
{
public:
    explicit DesktopDeviceConfigurationWidget(const IDevicePtr &device);

    void updateDeviceFromUi() override;

private:
    void updateFreePorts();

    void initGui();

private:
    QLineEdit *m_freePortsLineEdit;
    QLabel *m_portsWarningLabel;
};

} // namespace ProjectExplorer
