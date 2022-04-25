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

#include "mcutargetfactory.h"
#include "mcuhelpers.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QVersionNumber>

namespace McuSupport::Internal {

using Sdk::McuTargetDescription;
using Sdk::PackageDescription;

bool isToolchainDescriptionValid(const McuTargetDescription::Toolchain &t)
{
    return !t.id.isEmpty() && !t.compiler.cmakeVar.isEmpty() && !t.file.cmakeVar.isEmpty();
}

bool isDesktopToolchain(McuToolChainPackage::ToolChainType type)
{
    return type == McuToolChainPackage::ToolChainType::MSVC
           || type == McuToolChainPackage::ToolChainType::GCC;
}

const static QMap<QString, McuToolChainPackage::ToolChainType> toolchainTypeMapping{
    {"iar", McuToolChainPackage::ToolChainType::IAR},
    {"keil", McuToolChainPackage::ToolChainType::KEIL},
    {"msvc", McuToolChainPackage::ToolChainType::MSVC},
    {"gcc", McuToolChainPackage::ToolChainType::GCC},
    {"armgcc", McuToolChainPackage::ToolChainType::ArmGcc},
    {"ghs", McuToolChainPackage::ToolChainType::GHS},
    {"ghsarm", McuToolChainPackage::ToolChainType::GHSArm},
};

McuTargetFactory::McuTargetFactory(const SettingsHandler::Ptr &settingsHandler)
    : settingsHandler{settingsHandler}
{}

QPair<Targets, Packages> McuTargetFactory::createTargets(const McuTargetDescription &desc,
                                                         const Utils::FilePath & /*qtForMCUSdkPath*/)
{
    Targets mcuTargets;
    Packages packages;

    for (int colorDepth : desc.platform.colorDepths) {
        const McuTarget::Platform platform(
            {desc.platform.id, desc.platform.name, desc.platform.vendor});

        auto *toolchain = createToolchain(desc.toolchain);
        McuPackagePtr toolchainFile{createPackage(desc.toolchain.file)};
        //Skip target with incorrect toolchain dir or toolchain file.
        if (!toolchain || !toolchainFile)
            continue;
        Packages targetPackages = createPackages(desc);
        packages.unite(targetPackages);
        mcuTargets.append(McuTargetPtr{new McuTarget{QVersionNumber::fromString(desc.qulVersion),
                                                     platform,
                                                     deduceOperatingSystem(desc),
                                                     targetPackages,
                                                     McuToolChainPackagePtr{toolchain},
                                                     toolchainFile,
                                                     colorDepth}});
    }
    return {mcuTargets, packages};
}

QList<PackageDescription> aggregatePackageEntries(const McuTargetDescription &desc)
{
    QList<PackageDescription> result;
    result.append(desc.boardSdk.packages);
    result.append(desc.freeRTOS.packages);
    return result;
}

Packages McuTargetFactory::createPackages(const McuTargetDescription &desc)
{
    Packages packages;
    QList<PackageDescription> packageDescriptions = aggregatePackageEntries(desc);

    for (const PackageDescription &pkgDesc : packageDescriptions) {
        packages.insert(createPackage(pkgDesc));
    }

    return packages;
}

McuPackagePtr McuTargetFactory::createPackage(const PackageDescription &pkgDesc)
{
    return McuPackagePtr{new McuPackage{
        settingsHandler,
        pkgDesc.label,
        pkgDesc.defaultPath,
        pkgDesc.validationPath,
        pkgDesc.setting,
        pkgDesc.cmakeVar,
        pkgDesc.envVar,
    }};
}

McuToolChainPackage *McuTargetFactory::createToolchain(
    const McuTargetDescription::Toolchain &toolchain)
{
    const PackageDescription compilerDescription{toolchain.compiler};

    McuToolChainPackage::ToolChainType toolchainType
        = toolchainTypeMapping.value(toolchain.id, McuToolChainPackage::ToolChainType::Unsupported);

    if (isDesktopToolchain(toolchainType))
        return new McuToolChainPackage{settingsHandler, {}, {}, {}, {}, toolchainType};
    else if (!isToolchainDescriptionValid(toolchain))
        toolchainType = McuToolChainPackage::ToolChainType::Unsupported;

    return new McuToolChainPackage{
        settingsHandler,
        compilerDescription.label,
        compilerDescription.defaultPath,
        compilerDescription.validationPath,
        compilerDescription.setting,
        toolchainType,
        compilerDescription.cmakeVar,
        compilerDescription.envVar,
        nullptr, // McuPackageVersionDetector
    };
}

} // namespace McuSupport::Internal
