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

#include "mcutargetfactorylegacy.h"
#include "mcuhelpers.h"
#include "mcupackage.h"
#include "mcusupportsdk.h"
#include "mcusupportversiondetection.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"

#include <utils/fileutils.h>
#include <QVersionNumber>

namespace McuSupport::Internal {

McuTargetFactoryLegacy::McuTargetFactoryLegacy(
    const QHash<QString, ToolchainCompilerCreator> &toolchainCreators,
    const QHash<QString, McuPackagePtr> &toolchainFiles,
    const QHash<QString, McuPackagePtr> &vendorPkgs,
    const SettingsHandler::Ptr &settingsHandler)
    : toolchainCreators(toolchainCreators)
    , toolchainFiles(toolchainFiles)
    , vendorPkgs(vendorPkgs)
    , settingsHandler(settingsHandler)
{}

QPair<Targets, Packages> McuTargetFactoryLegacy::createTargets(const Sdk::McuTargetDescription &desc,
                                                               const Utils::FilePath &qtForMcuPath)
{
    QHash<QString, McuPackagePtr> boardSdkPkgs;
    QHash<QString, McuPackagePtr> freeRTOSPkgs;
    Targets mcuTargets;
    Packages packages;
    McuToolChainPackagePtr toolchainPackage = getToolchainCompiler(desc.toolchain);
    McuPackagePtr toolchainFilePackage = getToolchainFile(qtForMcuPath, desc.toolchain.id);
    for (int colorDepth : desc.platform.colorDepths) {
        Packages required3rdPartyPkgs;
        // Desktop toolchains don't need any additional settings
        if (toolchainPackage && !toolchainPackage->isDesktopToolchain()
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
                McuPackagePtr boardSdkPkg{createBoardSdkPackage(settingsHandler, desc)};
                boardSdkPkgs.insert(desc.boardSdk.envVar, boardSdkPkg);
            }
            McuPackagePtr boardSdkPkg{boardSdkPkgs.value(desc.boardSdk.envVar)};
            boardSdkPkg->setVersions(desc.boardSdk.versions);
            boardSdkDefaultPath = boardSdkPkg->defaultPath();
            required3rdPartyPkgs.insert(boardSdkPkg);
        }

        // Free RTOS specific settings.
        if (!desc.freeRTOS.envVar.isEmpty()) {
            if (!freeRTOSPkgs.contains(desc.freeRTOS.envVar)) {
                freeRTOSPkgs
                    .insert(desc.freeRTOS.envVar,
                            McuPackagePtr{
                                Sdk::createFreeRTOSSourcesPackage(settingsHandler,
                                                                  desc.freeRTOS.envVar,
                                                                  boardSdkDefaultPath,
                                                                  desc.freeRTOS.boardSdkSubDir)});
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

McuAbstractTargetFactory::AdditionalPackages McuTargetFactoryLegacy::getAdditionalPackages() const
{
    return {{}, vendorPkgs};
}

McuToolChainPackagePtr McuTargetFactoryLegacy::getToolchainCompiler(
    const Sdk::McuTargetDescription::Toolchain &desc) const
{
    auto compilerCreator = toolchainCreators.value(desc.id, [this] {
        return McuToolChainPackagePtr{Sdk::createUnsupportedToolChainPackage(settingsHandler)};
    });
    McuToolChainPackagePtr toolchainPackage = compilerCreator();
    toolchainPackage->setVersions(desc.versions);
    return toolchainPackage;
}

McuPackagePtr McuTargetFactoryLegacy::getToolchainFile(const Utils::FilePath &qtForMCUSdkPath,
                                                       const QString &toolchainName) const
{
    if (McuPackagePtr toolchainFile = toolchainFiles.value(toolchainName); toolchainFile) {
        return toolchainFile;
    } else {
        return McuPackagePtr{
            Sdk::createUnsupportedToolChainFilePackage(settingsHandler, qtForMCUSdkPath)};
    }
}
} // namespace McuSupport::Internal
