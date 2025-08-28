// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <utils/filepath.h>

namespace RemoteLinux::Internal {

// Asks for public key and returns true if the file dialog is not canceled.
bool runPublicKeyDeploymentDialog(const ProjectExplorer::DeviceConstRef &device,
                                  const Utils::FilePath &publicKeyFilePath = {});

} // namespace RemoteLinux::Internal
