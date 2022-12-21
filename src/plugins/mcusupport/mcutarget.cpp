// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcutarget.h"
#include "mcukitmanager.h"
#include "mcupackage.h"
#include "mcusupportplugin.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace McuSupport::Internal {

McuTarget::McuTarget(const QVersionNumber &qulVersion,
                     const Platform &platform,
                     OS os,
                     const Packages &packages,
                     const McuToolChainPackagePtr &toolChainPackage,
                     const McuPackagePtr &toolChainFilePackage,
                     int colorDepth)
    : m_qulVersion(qulVersion)
    , m_platform(platform)
    , m_os(os)
    , m_packages(packages)
    , m_toolChainPackage(toolChainPackage)
    , m_toolChainFilePackage(toolChainFilePackage)
    , m_colorDepth(colorDepth)
{}

Packages McuTarget::packages() const
{
    return m_packages;
}

McuToolChainPackagePtr McuTarget::toolChainPackage() const
{
    return m_toolChainPackage;
}

McuPackagePtr McuTarget::toolChainFilePackage() const
{
    return m_toolChainFilePackage;
}

McuTarget::OS McuTarget::os() const
{
    return m_os;
}

McuTarget::Platform McuTarget::platform() const
{
    return m_platform;
}

bool McuTarget::isValid() const
{
    return Utils::allOf(packages(), [](const McuPackagePtr &package) {
        package->updateStatus();
        return package->isValidStatus();
    });
}

QString McuTarget::desktopCompilerId() const
{
    // MinGW shares CMake configuration with GCC
    // and it is distinguished from MSVC by CMake compiler ID.
    // This provides the compiler ID to set up a different Qul configuration
    // for MSVC and MinGW.
    if (m_toolChainPackage) {
        switch (m_toolChainPackage->toolchainType()) {
        case McuToolChainPackage::ToolChainType::MSVC:
            return QLatin1String("msvc");
        case McuToolChainPackage::ToolChainType::GCC:
        case McuToolChainPackage::ToolChainType::MinGW:
            return QLatin1String("gnu");
        default:
            return QLatin1String("unsupported");
        }
    }
    return QLatin1String("invalid");
}

void McuTarget::printPackageProblems() const
{
    for (auto package : packages()) {
        package->updateStatus();
        if (!package->isValidStatus())
            printMessage(tr("Error creating kit for target %1, package %2: %3")
                             .arg(McuKitManager::generateKitNameFromTarget(this),
                                  package->label(),
                                  package->statusText()),
                         true);
        if (package->status() == McuAbstractPackage::Status::ValidPackageMismatchedVersion)
            printMessage(tr("Warning creating kit for target %1, package %2: %3")
                             .arg(McuKitManager::generateKitNameFromTarget(this),
                                  package->label(),
                                  package->statusText()),
                         false);
    }
}

QVersionNumber McuTarget::qulVersion() const
{
    return m_qulVersion;
}

int McuTarget::colorDepth() const
{
    return m_colorDepth;
}

} // namespace McuSupport::Internal
