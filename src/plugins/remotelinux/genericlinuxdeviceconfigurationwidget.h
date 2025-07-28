// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicewidget.h>

namespace RemoteLinux::Internal {

ProjectExplorer::IDeviceWidget *createLinuxDeviceWidget(const ProjectExplorer::IDevicePtr &device);

} // RemoteLinux::Internal
