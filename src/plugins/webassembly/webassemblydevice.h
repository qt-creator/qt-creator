// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>

namespace WebAssembly::Internal {

ProjectExplorer::IDevicePtr createWebAssemblyDevice();

void setupWebAssemblyDevice();

} // WebAssembly::Interenal
