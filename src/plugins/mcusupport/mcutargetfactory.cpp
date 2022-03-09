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
#include "mcutarget.h"
#include "mcutargetdescription.h"

#include <QVersionNumber>

namespace McuSupport::Internal::Sdk {

QPair<Targets, Packages> McuTargetFactory::createTargets(const McuTargetDescription &desc)
{
    Targets mcuTargets;
    Packages packages;

    for (int colorDepth : desc.platform.colorDepths) {
        const McuTarget::Platform platform(
            {desc.platform.id, desc.platform.name, desc.platform.vendor});

        Packages targetPackages = createPackages(desc);
        packages.append(targetPackages);
        mcuTargets.emplace_back(new McuTarget{QVersionNumber::fromString(desc.qulVersion),
                                              platform,
                                              deduceOperatingSystem(desc),
                                              targetPackages,
                                              new McuToolChainPackage{{}, {}, {}, {}, {}},
                                              colorDepth});
    }
    return {mcuTargets, packages};
}

QList<PackageDescription> aggregatePackageEntries(const McuTargetDescription &desc)
{
    QList<PackageDescription> result;
    result.append(desc.boardSdk.packages);
    result.append(desc.freeRTOS.packages);
    result.append(desc.toolchain.packages);
    return result;
}

Packages McuTargetFactory::createPackages(const McuTargetDescription &desc)
{
    Packages packages;
    QList<PackageDescription> packageDescriptions = aggregatePackageEntries(desc);

    for (const PackageDescription &pkgDesc : packageDescriptions) {
        packages.emplace_back(new McuPackage{
            pkgDesc.label,
            pkgDesc.defaultPath,
            pkgDesc.validationPath,
            pkgDesc.setting,
            pkgDesc.cmakeVar,
            pkgDesc.envVar,
        });
    }

    return packages;
}

} // namespace McuSupport::Internal::Sdk
