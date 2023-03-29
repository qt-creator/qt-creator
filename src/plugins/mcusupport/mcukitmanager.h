// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcusupport_global.h"
#include "settingshandler.h"

#include <utils/environmentfwd.h>

#include <QCoreApplication>
#include <QVersionNumber>

namespace ProjectExplorer {
class Kit;
} // namespace ProjectExplorer

namespace McuSupport::Internal {

class McuAbstractPackage;
class McuTarget;

namespace McuKitManager {
enum class UpgradeOption { Ignore, Keep, Replace };

// Kit Factory
ProjectExplorer::Kit *newKit(const McuTarget *mcuTarget, const McuPackagePtr &qtForMCUsSdk);

// Kit information
QString generateKitNameFromTarget(const McuTarget *mcuTarget);
QVersionNumber kitQulVersion(const ProjectExplorer::Kit *kit);
bool kitIsUpToDate(const ProjectExplorer::Kit *kit,
                   const McuTarget *mcuTarget,
                   const McuPackagePtr &qtForMCUsSdkPackage);

// Queries
QList<ProjectExplorer::Kit *> existingKits(const McuTarget *mcuTarget);
QList<ProjectExplorer::Kit *> matchingKits(const McuTarget *mcuTarget,
                                           const McuPackagePtr &qtForMCUsSdkPackage);
QList<ProjectExplorer::Kit *> upgradeableKits(const McuTarget *mcuTarget,
                                              const McuPackagePtr &qtForMCUsSdkPackage);
QList<ProjectExplorer::Kit *> kitsWithMismatchedDependencies(const McuTarget *mcuTarget);
QList<ProjectExplorer::Kit *> outdatedKits();

// Maintenance
void createAutomaticKits(const SettingsHandler::Ptr &);
void upgradeKitsByCreatingNewPackage(const SettingsHandler::Ptr &, UpgradeOption upgradeOption);
void upgradeKitInPlace(ProjectExplorer::Kit *kit,
                       const McuTarget *mcuTarget,
                       const McuPackagePtr &qtForMCUsSdk);

// Fixing kits:
void updatePathsInExistingKits(const SettingsHandler::Ptr &);
void fixExistingKits(const SettingsHandler::Ptr &);

// Outdated kits:
void removeOutdatedKits();

// kits for uninstalled targets:
const QList<ProjectExplorer::Kit *> findUninstalledTargetsKits();
void removeUninstalledTargetsKits(const QList<ProjectExplorer::Kit *> uninstalledTargetsKits);

} // namespace McuKitManager
} // namespace McuSupport::Internal

Q_DECLARE_METATYPE(McuSupport::Internal::McuKitManager::UpgradeOption)
