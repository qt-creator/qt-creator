// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicewidget.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QRadioButton;
class QSpinBox;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class FilePath;
class PathChooser;
} // Utils

namespace RemoteLinux::Internal {

class GenericLinuxDeviceConfigurationWidget
        : public ProjectExplorer::IDeviceWidget
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceConfigurationWidget(const ProjectExplorer::IDevicePtr &device);
    ~GenericLinuxDeviceConfigurationWidget() override;

private:
    void authenticationTypeChanged();
    void hostNameEditingFinished();
    void sshPortEditingFinished();
    void timeoutEditingFinished();
    void userNameEditingFinished();
    void keyFileEditingFinished();
    void gdbServerEditingFinished();
    void qmlRuntimeEditingFinished();
    void handleFreePortsChanged();
    void setPrivateKey(const Utils::FilePath &path);
    void createNewKey();
    void hostKeyCheckingChanged(bool doCheck);
    void sourceProfileCheckingChanged(bool doCheck);
    void linkDeviceChanged(int index);

    void updateDeviceFromUi() override;
    void updatePortsWarningLabel();
    void initGui();

    QRadioButton *m_defaultAuthButton;
    QLabel *m_keyLabel;
    QRadioButton *m_keyButton;
    Utils::FancyLineEdit *m_hostLineEdit;
    QSpinBox *m_sshPortSpinBox;
    QCheckBox *m_hostKeyCheckBox;
    Utils::FancyLineEdit *m_portsLineEdit;
    QLabel *m_portsWarningLabel;
    Utils::FancyLineEdit *m_userLineEdit;
    QSpinBox *m_timeoutSpinBox;
    Utils::PathChooser *m_keyFileLineEdit;
    QLabel *m_machineTypeValueLabel;
    Utils::PathChooser *m_gdbServerLineEdit;
    Utils::PathChooser *m_qmlRuntimeLineEdit;
    QCheckBox *m_sourceProfileCheckBox;
    QComboBox *m_linkDeviceComboBox;
};

} // RemoteLinux::Internal
