// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "dockerdevice.h"

#include "kitdetector.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicewidget.h>

#include <utils/pathchooser.h>
#include <utils/pathlisteditor.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QLineEdit;
class QToolButton;
QT_END_NAMESPACE

namespace Docker::Internal {

class DockerDeviceWidget final : public ProjectExplorer::IDeviceWidget
{
public:
    explicit DockerDeviceWidget(const ProjectExplorer::IDevice::Ptr &device);

    void updateDeviceFromUi() final {}
    void updateDaemonStateTexts();

private:
    QLineEdit *m_repoLineEdit;
    QLineEdit *m_tagLineEdit;
    QLineEdit *m_idLineEdit;
    QToolButton *m_daemonReset;
    QLabel *m_daemonState;
    QCheckBox *m_runAsOutsideUser;
    QCheckBox *m_keepEntryPoint;
    QCheckBox *m_enableLldbFlags;
    Utils::PathChooser *m_clangdExecutable;

    Utils::PathListEditor *m_pathsListEdit;
    KitDetector m_kitItemDetector;

    DockerDeviceData m_data;
};

} // Docker::Internal
