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
    void createNewKey();
    void updateDeviceFromUi() override {}
};

} // RemoteLinux::Internal
