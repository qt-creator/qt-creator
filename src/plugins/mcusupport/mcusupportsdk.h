/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "mcusupport_global.h"
#include "settingshandler.h"

#include <utils/filepath.h>

#include <QSettings>

namespace Utils {
class FilePath;
} // namespace Utils

namespace McuSupport::Internal {

constexpr int MAX_COMPATIBILITY_VERSION{1};

class McuAbstractPackage;
class McuPackage;
class McuSdkRepository;
class McuTarget;
class McuToolChainPackage;

namespace Sdk {

struct McuTargetDescription;

McuPackagePtr createQtForMCUsPackage(const SettingsHandler::Ptr &);

bool checkDeprecatedSdkError(const Utils::FilePath &qulDir, QString &message);

McuSdkRepository targetsAndPackages(const Utils::FilePath &qulDir, const SettingsHandler::Ptr &);

McuTargetDescription parseDescriptionJson(const QByteArray &);
McuSdkRepository targetsFromDescriptions(const QList<McuTargetDescription> &,
                                         const SettingsHandler::Ptr &,
                                         const Utils::FilePath &qtForMCUSdkPath,
                                         bool isLegacy);

Utils::FilePath kitsPath(const Utils::FilePath &dir);

McuPackagePtr createUnsupportedToolChainFilePackage(const SettingsHandler::Ptr &,
                                                    const Utils::FilePath &qtMcuSdkPath);
McuToolChainPackagePtr createUnsupportedToolChainPackage(const SettingsHandler::Ptr &);
McuToolChainPackagePtr createIarToolChainPackage(const SettingsHandler::Ptr &);
McuToolChainPackagePtr createGccToolChainPackage(const SettingsHandler::Ptr &);
McuToolChainPackagePtr createArmGccToolchainPackage(const SettingsHandler::Ptr &);
McuToolChainPackagePtr createMsvcToolChainPackage(const SettingsHandler::Ptr &);
McuToolChainPackagePtr createGhsToolchainPackage(const SettingsHandler::Ptr &);
McuToolChainPackagePtr createGhsArmToolchainPackage(const SettingsHandler::Ptr &);

McuPackagePtr createBoardSdkPackage(const SettingsHandler::Ptr &, const McuTargetDescription &);
McuPackagePtr createFreeRTOSSourcesPackage(const SettingsHandler::Ptr &settingsHandler,
                                           const QString &envVar,
                                           const Utils::FilePath &boardSdkDir,
                                           const Utils::FilePath &freeRTOSBoardSdkSubDir);

} // namespace Sdk
} // namespace McuSupport::Internal
