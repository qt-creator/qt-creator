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

#include <utils/filepath.h>

#include <QPair>
#include <QSettings>

namespace McuSupport::Internal {

constexpr int MAX_COMPATIBILITY_VERSION{1};

class McuSdkRepository;
class McuAbstractPackage;
class McuToolChainPackage;
class McuTarget;

namespace Sdk {

struct McuTargetDescription;

McuAbstractPackage *createQtForMCUsPackage();

bool checkDeprecatedSdkError(const Utils::FilePath &qulDir, QString &message);

void targetsAndPackages(const Utils::FilePath &qulDir, McuSdkRepository *repo);

McuTargetDescription parseDescriptionJson(const QByteArray &);
QPair<Targets, Packages> targetsFromDescriptions(const QList<McuTargetDescription> &, bool isLegacy);

Utils::FilePath kitsPath(const Utils::FilePath &dir);

Utils::FilePath packagePathFromSettings(const QString &settingsKey,
                                        QSettings::Scope scope,
                                        const Utils::FilePath &defaultPath);

McuToolChainPackage *createUnsupportedToolChainPackage();
McuAbstractPackage *createBoardSdkPackage(const McuTargetDescription &desc);
McuAbstractPackage *createFreeRTOSSourcesPackage(const QString &envVar,
                                                 const Utils::FilePath &boardSdkDir,
                                                 const QString &freeRTOSBoardSdkSubDir);

} // namespace Sdk
} // namespace McuSupport::Internal
