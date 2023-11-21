// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androiddeviceinfo.h"

namespace Android {
namespace Internal {

const char avdManufacturerError[] = "no longer exists as a device";

AndroidDeviceInfoList parseAvdList(const QString &output, Utils::FilePaths *avdErrorPaths);
int platformNameToApiLevel(const QString &platformName);
QString convertNameToExtension(const QString &name);

} // namespace Internal
} // namespace Android
