// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevicefwd.h"
#include "../projectexplorer_export.h"

#include <utils/id.h>

namespace Utils { class FilePath; }

namespace ProjectExplorer {
class Kit;

class PROJECTEXPLORER_EXPORT DeviceTypeKitAspect
{
public:
    static const Utils::Id id();
    static const Utils::Id deviceTypeId(const Kit *k);
    static void setDeviceTypeId(Kit *k, Utils::Id type);
};

class PROJECTEXPLORER_EXPORT DeviceKitAspect
{
public:
    static Utils::Id id();
    static IDeviceConstPtr device(const Kit *k);
    static Utils::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDeviceConstPtr dev);
    static void setDeviceId(Kit *k, Utils::Id dataId);
    static Utils::FilePath deviceFilePath(const Kit *k, const QString &pathOnDevice);
};

class PROJECTEXPLORER_EXPORT BuildDeviceKitAspect
{
public:
    static Utils::Id id();
    static IDeviceConstPtr device(const Kit *k);
    static Utils::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDeviceConstPtr dev);
    static void setDeviceId(Kit *k, Utils::Id dataId);
};

} // namespace ProjectExplorer
