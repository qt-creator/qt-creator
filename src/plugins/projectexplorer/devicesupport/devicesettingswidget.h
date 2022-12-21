// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefwd.h"

#include <QList>
#include <QString>
#include <QWidget>

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
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

    QLabel *m_configurationLabel;
    QComboBox *m_configurationComboBox;
    QGroupBox *m_generalGroupBox;
    QLineEdit *m_nameLineEdit;
    QLabel *m_osTypeValueLabel;
    QLabel *m_autoDetectionLabel;
    QLabel *m_deviceStateIconLabel;
    QLabel *m_deviceStateTextLabel;
    QGroupBox *m_osSpecificGroupBox;
    QPushButton *m_addConfigButton;
    QPushButton *m_removeConfigButton;
    QPushButton *m_defaultDeviceButton;
    QVBoxLayout *m_buttonsLayout;
};

} // namespace Internal
} // namespace ProjectExplorer
