// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcusupport_global.h"
#include "settingshandler.h"
#include <utils/fileutils.h>

namespace McuSupport::Internal {

constexpr int MAX_COMPATIBILITY_VERSION{1};

class McuAbstractPackage;
class McuPackage;
class McuSdkRepository;
class McuTarget;
class McuToolChainPackage;
struct McuTargetDescription;

McuPackagePtr createQtForMCUsPackage(const SettingsHandler::Ptr &);

bool checkDeprecatedSdkError(const Utils::FilePath &qulDir, QString &message);

McuSdkRepository targetsAndPackages(const McuPackagePtr &qtForMCUsPackage,
                                    const SettingsHandler::Ptr &);

McuTargetDescription parseDescriptionJson(const QByteArray &, const Utils::FilePath &sourceFile = Utils::FilePath());
McuSdkRepository targetsFromDescriptions(const QList<McuTargetDescription> &,
                                         const SettingsHandler::Ptr &,
                                         const McuPackagePtr &qtForMCUsPackage,
                                         bool isLegacy);

Utils::FilePath kitsPath(const Utils::FilePath &dir);

namespace Legacy {

McuPackagePtr createUnsupportedToolChainFilePackage(const SettingsHandler::Ptr &,
                                                    const Utils::FilePath &qtMcuSdkPath);
McuToolChainPackagePtr createUnsupportedToolChainPackage(const SettingsHandler::Ptr &);
McuToolChainPackagePtr createIarToolChainPackage(const SettingsHandler::Ptr &, const QStringList &);
McuToolChainPackagePtr createGccToolChainPackage(const SettingsHandler::Ptr &, const QStringList &);
McuToolChainPackagePtr createArmGccToolchainPackage(const SettingsHandler::Ptr &,
                                                    const QStringList &);
McuToolChainPackagePtr createMsvcToolChainPackage(const SettingsHandler::Ptr &, const QStringList &);
McuToolChainPackagePtr createGhsToolchainPackage(const SettingsHandler::Ptr &, const QStringList &);
McuToolChainPackagePtr createGhsArmToolchainPackage(const SettingsHandler::Ptr &,
                                                    const QStringList &);

McuPackagePtr createBoardSdkPackage(const SettingsHandler::Ptr &, const McuTargetDescription &);
McuPackagePtr createFreeRTOSSourcesPackage(const SettingsHandler::Ptr &settingsHandler,
                                           const QString &envVar,
                                           const Utils::FilePath &boardSdkDir);

McuPackagePtr createStm32CubeProgrammerPackage(const SettingsHandler::Ptr &);
McuPackagePtr createRenesasProgrammerPackage(const SettingsHandler::Ptr &);
McuPackagePtr createCypressProgrammerPackage(const SettingsHandler::Ptr &);
McuPackagePtr createMcuXpressoIdePackage(const SettingsHandler::Ptr &);

} // namespace Legacy
} // namespace McuSupport::Internal
