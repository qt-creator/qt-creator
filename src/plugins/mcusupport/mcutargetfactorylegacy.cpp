// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcutargetfactorylegacy.h"
#include "mcuhelpers.h"
#include "mcupackage.h"
#include "mcusupportsdk.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"

#include <utils/fileutils.h>
#include <QVersionNumber>

namespace McuSupport::Internal::Legacy {

McuTargetFactory::McuTargetFactory(const QHash<QString, ToolchainCompilerCreator> &toolchainCreators,
                                   const QHash<QString, McuPackagePtr> &toolchainFiles,
                                   const QHash<QString, McuPackagePtr> &vendorPkgs,
                                   const SettingsHandler::Ptr &settingsHandler)
    : toolchainCreators(toolchainCreators)
    , toolchainFiles(toolchainFiles)
    , vendorPkgs(vendorPkgs)
    , settingsHandler(settingsHandler)
{}

QPair<Targets, Packages> McuTargetFactory::createTargets(const McuTargetDescription &desc,
                                                         const McuPackagePtr &qtForMCUsPackage)
{
    const Utils::FilePath &qtForMcuPath{qtForMCUsPackage->path()};
    QHash<QString, McuPackagePtr> boardSdkPkgs;
    QHash<QString, McuPackagePtr> freeRTOSPkgs;
    Targets mcuTargets;
    Packages packages;
    McuToolChainPackagePtr toolchainPackage = getToolchainCompiler(desc.toolchain);
    McuPackagePtr toolchainFilePackage = getToolchainFile(qtForMcuPath, desc.toolchain.id);
    for (int colorDepth : desc.platform.colorDepths) {
        Packages required3rdPartyPkgs;
        // Desktop toolchains don't need any additional settings
        if (toolchainPackage
            && toolchainPackage->toolchainType()
                   != McuToolChainPackage::ToolChainType::Unsupported) {
            required3rdPartyPkgs.insert(toolchainPackage);
        }

        //  Add setting specific to platform IDE.
        if (vendorPkgs.contains(desc.platform.vendor)) {
            required3rdPartyPkgs.insert(vendorPkgs.value(desc.platform.vendor));
        }

        // Board SDK specific settings
        Utils::FilePath boardSdkDefaultPath;
        if (!desc.boardSdk.envVar.isEmpty()) {
            if (!boardSdkPkgs.contains(desc.boardSdk.envVar)) {
                McuPackagePtr boardSdkPkg{Legacy::createBoardSdkPackage(settingsHandler, desc)};
                boardSdkPkgs.insert(desc.boardSdk.envVar, boardSdkPkg);
            }
            McuPackagePtr boardSdkPkg{boardSdkPkgs.value(desc.boardSdk.envVar)};
            boardSdkDefaultPath = boardSdkPkg->path();
            required3rdPartyPkgs.insert(boardSdkPkg);
        }

        // Free RTOS specific settings.
        if (!desc.freeRTOS.envVar.isEmpty()) {
            if (!freeRTOSPkgs.contains(desc.freeRTOS.envVar)) {
                freeRTOSPkgs.insert(desc.freeRTOS.envVar,
                                    McuPackagePtr{
                                        Legacy::createFreeRTOSSourcesPackage(settingsHandler,
                                                                             desc.freeRTOS.envVar,
                                                                             boardSdkDefaultPath)});
            }
            required3rdPartyPkgs.insert(freeRTOSPkgs.value(desc.freeRTOS.envVar));
        }

        packages.unite(required3rdPartyPkgs);
        const McuTarget::Platform platform(
            {desc.platform.id, desc.platform.name, desc.platform.vendor});
        mcuTargets.append(McuTargetPtr{new McuTarget{QVersionNumber::fromString(desc.qulVersion),
                                                     platform,
                                                     deduceOperatingSystem(desc),
                                                     required3rdPartyPkgs,
                                                     toolchainPackage,
                                                     toolchainFilePackage,
                                                     colorDepth}});
    }
    return {mcuTargets, packages};
}

McuAbstractTargetFactory::AdditionalPackages McuTargetFactory::getAdditionalPackages() const
{
    return {{}, vendorPkgs};
}

McuToolChainPackagePtr McuTargetFactory::getToolchainCompiler(
    const McuTargetDescription::Toolchain &desc) const
{
    auto compilerCreator = toolchainCreators.value(desc.id, [this](const QStringList & /*versions*/) {
        return McuToolChainPackagePtr{Legacy::createUnsupportedToolChainPackage(settingsHandler)};
    });
    McuToolChainPackagePtr toolchainPackage = compilerCreator(desc.versions);
    return toolchainPackage;
}

McuPackagePtr McuTargetFactory::getToolchainFile(const Utils::FilePath &qtForMCUSdkPath,
                                                 const QString &toolchainName) const
{
    if (McuPackagePtr toolchainFile = toolchainFiles.value(toolchainName); toolchainFile) {
        return toolchainFile;
    } else {
        return McuPackagePtr{
            Legacy::createUnsupportedToolChainFilePackage(settingsHandler, qtForMCUSdkPath)};
    }
}
} // namespace McuSupport::Internal::Legacy
