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

#pragma once

#include <utils/environmentfwd.h>
#include "mcusupport_global.h"

#include <QCoreApplication>
#include <QObject>
#include <QVector>
#include <QVersionNumber>

namespace ProjectExplorer {
class Kit;
} // namespace ProjectExplorer

namespace McuSupport {
namespace Internal {

class McuAbstractPackage;
class McuToolChainPackage;
class McuTarget;

namespace McuKitManager
{
    enum class UpgradeOption {
        Ignore,
        Keep,
        Replace
    };

    // Creating kits:
    ProjectExplorer::Kit *newKit(const McuTarget *mcuTarget, const McuAbstractPackage *qtForMCUsSdk);
    void createAutomaticKits();

    // Querying the kits:
    QList<ProjectExplorer::Kit *> existingKits(const McuTarget *mcuTarget);
    QList<ProjectExplorer::Kit *> matchingKits(const McuTarget *mcuTarget,
                                                      const McuAbstractPackage *qtForMCUsSdkPackage);
    QList<ProjectExplorer::Kit *> upgradeableKits(
        const McuTarget *mcuTarget, const McuAbstractPackage *qtForMCUsSdkPackage);
    QList<ProjectExplorer::Kit *> kitsWithMismatchedDependencies(const McuTarget *mcuTarget);

    // Upgrading kits:
    void upgradeKitsByCreatingNewPackage(UpgradeOption upgradeOption);
    void upgradeKitInPlace(ProjectExplorer::Kit *kit, const McuTarget *mcuTarget, const McuAbstractPackage *qtForMCUsSdk);

    // Fixing kits:
    void fixKitsDependencies();
    void fixExistingKits();

    // Outdated kits:
    QList<ProjectExplorer::Kit *> outdatedKits();
    void removeOutdatedKits();

    // Querying kits:
    QString kitName(const McuTarget* mcuTarget);
    QVersionNumber kitQulVersion(const ProjectExplorer::Kit *kit);
    bool kitIsUpToDate(const ProjectExplorer::Kit *kit, const McuTarget *mcuTarget, const McuAbstractPackage *qtForMCUsSdkPackage);

} // namespace McuKitManager
} // namespace Internal
} // namespace McuSupport
