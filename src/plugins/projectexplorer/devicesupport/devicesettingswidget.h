/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "idevice.h"

#include <QList>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class IDevice;
class DeviceManager;
class DeviceManagerModel;
class IDeviceWidget;

namespace Internal {
namespace Ui { class DeviceSettingsWidget; }
class NameValidator;

class DeviceSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    DeviceSettingsWidget(QWidget *parent = 0);
    ~DeviceSettingsWidget() override;

    void saveSettings();

private:
    void handleDeviceUpdated(Core::Id id);
    void currentDeviceChanged(int index);
    void addDevice();
    void removeDevice();
    void deviceNameEditingFinished();
    void setDefaultDevice();
    void testDevice();
    void handleProcessListRequested();

    void initGui();
    void handleAdditionalActionRequest(Core::Id actionId);
    void displayCurrent();
    void setDeviceInfoWidgetsEnabled(bool enable);
    IDevice::ConstPtr currentDevice() const;
    int currentIndex() const;
    void clearDetails();
    QString parseTestOutput();
    void fillInValues();
    void updateDeviceFromUi();

    Ui::DeviceSettingsWidget *m_ui;
    DeviceManager * const m_deviceManager;
    DeviceManagerModel * const m_deviceManagerModel;
    NameValidator * const m_nameValidator;
    QList<QPushButton *> m_additionalActionButtons;
    IDeviceWidget *m_configWidget;
};

} // namespace Internal
} // namespace ProjectExplorer
