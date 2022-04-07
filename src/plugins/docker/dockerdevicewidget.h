/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "dockerplugin.h"
#include "dockerdevice.h"
#include "kitdetector.h"

#include <projectexplorer/devicesupport/idevicewidget.h>

#include <utils/pathlisteditor.h>

#include <QCheckBox>

namespace Docker {
namespace Internal {

class DockerDeviceWidget final : public ProjectExplorer::IDeviceWidget
{
    Q_DECLARE_TR_FUNCTIONS(Docker::Internal::DockerDevice)

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
    Utils::PathListEditor *m_pathsListEdit;
    KitDetector m_kitItemDetector;
};

} // Internal
} // Docker
