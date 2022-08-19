// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefwd.h"

#include <QList>
#include <QString>
#include <QWidget>

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class DeviceManager;
class DeviceManagerModel;
class IDeviceWidget;

namespace Internal {
namespace Ui { class DeviceSettingsWidget; }
class NameValidator;

class DeviceSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_OBJECT
public:
    DeviceSettingsWidget();
    ~DeviceSettingsWidget() final;

private:
    void apply() final { saveSettings(); }

    void saveSettings();

    void handleDeviceUpdated(Utils::Id id);
    void currentDeviceChanged(int index);
    void addDevice();
    void removeDevice();
    void deviceNameEditingFinished();
    void setDefaultDevice();
    void testDevice();
    void handleProcessListRequested();

    void initGui();
    void displayCurrent();
    void setDeviceInfoWidgetsEnabled(bool enable);
    IDeviceConstPtr currentDevice() const;
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
