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
#include "mcupackage.h"
#include "mcusupportsdk.h"
#include "mcusupportversiondetection.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"

#include <utils/fileutils.h>
#include <QVersionNumber>

namespace McuSupport::Internal::Sdk {

using namespace Utils;

QVector<McuAbstractPackage *> McuTargetFactory::getMcuPackages() const
{
    QVector<McuAbstractPackage *> packages;
    for (auto *package : qAsConst(boardSdkPkgs))
        packages.append(package);
    for (auto *package : qAsConst(freeRTOSPkgs))
        packages.append(package);
    return packages;
}

QVector<McuTarget *> McuTargetFactory::createTargets(const McuTargetDescription &desc)
{
    // OS deduction
    const auto os = [&] {
        if (desc.platform.type == McuTargetDescription::TargetType::Desktop)
            return McuTarget::OS::Desktop;
        else if (!desc.freeRTOS.envVar.isEmpty())
            return McuTarget::OS::FreeRTOS;
        return McuTarget::OS::BareMetal;
    }();

    QVector<McuTarget *> mcuTargets;
    McuToolChainPackage *tcPkg = tcPkgs.value(desc.toolchain.id);
    if (tcPkg)
        tcPkg->setVersions(desc.toolchain.versions);
    else
        tcPkg = createUnsupportedToolChainPackage();
    for (int colorDepth : desc.platform.colorDepths) {
        QVector<McuAbstractPackage *> required3rdPartyPkgs;
        // Desktop toolchains don't need any additional settings
        if (tcPkg && !tcPkg->isDesktopToolchain()
            && tcPkg->toolchainType() != McuToolChainPackage::ToolChainType::Unsupported)
            required3rdPartyPkgs.append(tcPkg);

        // Add setting specific to platform IDE
        if (vendorPkgs.contains(desc.platform.vendor))
            required3rdPartyPkgs.push_back(vendorPkgs.value(desc.platform.vendor));

        // Board SDK specific settings
        FilePath boardSdkDefaultPath;
        if (!desc.boardSdk.envVar.isEmpty()) {
            if (!boardSdkPkgs.contains(desc.boardSdk.envVar)) {
                auto boardSdkPkg = createBoardSdkPackage(desc);
                boardSdkPkgs.insert(desc.boardSdk.envVar, boardSdkPkg);
            }
            auto boardSdkPkg = boardSdkPkgs.value(desc.boardSdk.envVar);
            boardSdkPkg->setVersions(desc.boardSdk.versions);
            boardSdkDefaultPath = boardSdkPkg->defaultPath();
            required3rdPartyPkgs.append(boardSdkPkg);
        }

        // Free RTOS specific settings
        if (!desc.freeRTOS.envVar.isEmpty()) {
            if (!freeRTOSPkgs.contains(desc.freeRTOS.envVar)) {
                freeRTOSPkgs.insert(desc.freeRTOS.envVar,
                                    createFreeRTOSSourcesPackage(desc.freeRTOS.envVar,
                                                                 boardSdkDefaultPath,
                                                                 desc.freeRTOS.boardSdkSubDir));
            }
            required3rdPartyPkgs.append(freeRTOSPkgs.value(desc.freeRTOS.envVar));
        }

        const McuTarget::Platform platform(
            {desc.platform.id, desc.platform.name, desc.platform.vendor});
        auto mcuTarget = new McuTarget(QVersionNumber::fromString(desc.qulVersion),
                                       platform,
                                       os,
                                       required3rdPartyPkgs,
                                       tcPkg,
                                       colorDepth);
        mcuTargets.append(mcuTarget);
    }
    return mcuTargets;
}
} // namespace McuSupport::Internal::Sdk
