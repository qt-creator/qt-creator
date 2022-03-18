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

namespace McuSupport::Internal::Sdk {

QPair<Targets, Packages> McuTargetFactoryLegacy::createTargets(const McuTargetDescription &desc)
{
    QHash<QString, McuAbstractPackage *> boardSdkPkgs;
    QHash<QString, McuAbstractPackage *> freeRTOSPkgs;
    Targets mcuTargets;
    Packages packages;
    McuToolChainPackage *tcPkg = tcPkgs.value(desc.toolchain.id);
    if (tcPkg) {
        tcPkg->setVersions(desc.toolchain.versions);
    } else {
        tcPkg = createUnsupportedToolChainPackage();
    }
    for (int colorDepth : desc.platform.colorDepths) {
        Packages required3rdPartyPkgs;
        // Desktop toolchains don't need any additional settings
        if (tcPkg && !tcPkg->isDesktopToolchain()
            && tcPkg->toolchainType() != McuToolChainPackage::ToolChainType::Unsupported) {
            required3rdPartyPkgs.append(tcPkg);
        }

        //  Add setting specific to platform IDE.
        if (vendorPkgs.contains(desc.platform.vendor)) {
            required3rdPartyPkgs.append(vendorPkgs.value(desc.platform.vendor));
        }

        // Board SDK specific settings
        Utils::FilePath boardSdkDefaultPath;
        if (!desc.boardSdk.envVar.isEmpty()) {
            if (!boardSdkPkgs.contains(desc.boardSdk.envVar)) {
                McuAbstractPackage *boardSdkPkg = createBoardSdkPackage(desc);
                boardSdkPkgs.insert(desc.boardSdk.envVar, boardSdkPkg);
            }
            McuAbstractPackage *boardSdkPkg{boardSdkPkgs.value(desc.boardSdk.envVar)};
            boardSdkPkg->setVersions(desc.boardSdk.versions);
            boardSdkDefaultPath = boardSdkPkg->defaultPath();
            required3rdPartyPkgs.append(boardSdkPkg);
        }

        // Free RTOS specific settings.
        if (!desc.freeRTOS.envVar.isEmpty()) {
            if (!freeRTOSPkgs.contains(desc.freeRTOS.envVar)) {
                freeRTOSPkgs.insert(desc.freeRTOS.envVar,
                                     createFreeRTOSSourcesPackage(desc.freeRTOS.envVar,
                                                                  boardSdkDefaultPath,
                                                                  desc.freeRTOS.boardSdkSubDir));
            }
            required3rdPartyPkgs.append(freeRTOSPkgs.value(desc.freeRTOS.envVar));
        }

        packages.append(required3rdPartyPkgs);
        const McuTarget::Platform platform(
            {desc.platform.id, desc.platform.name, desc.platform.vendor});
        mcuTargets.push_back(new McuTarget{QVersionNumber::fromString(desc.qulVersion),
                                           platform,
                                           deduceOperatingSystem(desc),
                                           required3rdPartyPkgs,
                                           tcPkg,
                                           colorDepth});
    }
    return {mcuTargets, packages};
}

McuTargetFactoryLegacy::AdditionalPackages McuTargetFactoryLegacy::getAdditionalPackages() const
{
    return {tcPkgs, vendorPkgs};
}
} // namespace McuSupport::Internal::Sdk
