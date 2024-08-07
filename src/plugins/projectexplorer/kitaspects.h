// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abi.h"
#include "devicesupport/idevicefwd.h"
#include "kitmanager.h"
#include "kit.h"

#include <utils/environment.h>

namespace ProjectExplorer {
class Toolchain;
class ToolchainBundle;

// SysRootKitAspect

class PROJECTEXPLORER_EXPORT SysRootKitAspect
{
public:
    static Utils::Id id();
    static Utils::FilePath sysRoot(const Kit *k);
    static void setSysRoot(Kit *k, const Utils::FilePath &v);
};

// ToolchainKitAspect

class PROJECTEXPLORER_EXPORT ToolchainKitAspect
{
public:
    static Utils::Id id();
    static QByteArray toolchainId(const Kit *k, Utils::Id language);
    static Toolchain *toolchain(const Kit *k, Utils::Id language);
    static Toolchain *cToolchain(const Kit *k);
    static Toolchain *cxxToolchain(const Kit *k);
    static QList<Toolchain *> toolChains(const Kit *k);
    static void setToolchain(Kit *k, Toolchain *tc);
    static void setBundle(Kit *k, const ToolchainBundle &bundle);
    static void clearToolchain(Kit *k, Utils::Id language);
    static Abi targetAbi(const Kit *k);

    static QString msgNoToolchainInTarget();
};

// DeviceTypeKitAspect

class PROJECTEXPLORER_EXPORT DeviceTypeKitAspect
{
public:
    static const Utils::Id id();
    static const Utils::Id deviceTypeId(const Kit *k);
    static void setDeviceTypeId(Kit *k, Utils::Id type);
};

// DeviceKitAspect

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


// BuildDeviceKitAspect

class PROJECTEXPLORER_EXPORT BuildDeviceKitAspect
{
public:
    static Utils::Id id();
    static IDeviceConstPtr device(const Kit *k);
    static Utils::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDeviceConstPtr dev);
    static void setDeviceId(Kit *k, Utils::Id dataId);
};

// EnvironmentKitAspect

class PROJECTEXPLORER_EXPORT EnvironmentKitAspect
{
public:
    static Utils::Id id();
    static Utils::EnvironmentItems environmentChanges(const Kit *k);
    static void setEnvironmentChanges(Kit *k, const Utils::EnvironmentItems &changes);
};

} // namespace ProjectExplorer
