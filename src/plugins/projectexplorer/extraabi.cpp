/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "extraabi.h"

#include "abi.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/settingsaccessor.h>

#include <app/app_version.h>

#include <QDebug>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

class AbiFlavorUpgraderV0 : public VersionUpgrader
{
public:
    AbiFlavorUpgraderV0() : VersionUpgrader(0, "") { }

    QVariantMap upgrade(const QVariantMap &data) override { return data; }
};

class AbiFlavorAccessor : public UpgradingSettingsAccessor
{
public:
    AbiFlavorAccessor();
};

AbiFlavorAccessor::AbiFlavorAccessor() :
    UpgradingSettingsAccessor("QtCreatorExtraAbi",
                              QCoreApplication::translate("ProjectExplorer::ToolChainManager", "ABI"),
                              Core::Constants::IDE_DISPLAY_NAME)
{
    setBaseFilePath(FilePath::fromString(Core::ICore::installerResourcePath() + "/abi.xml"));

    addVersionUpgrader(std::make_unique<AbiFlavorUpgraderV0>());
}

// --------------------------------------------------------------------
// ExtraAbi:
// --------------------------------------------------------------------

void ExtraAbi::load()
{
    AbiFlavorAccessor accessor;
    const QVariantMap data = accessor.restoreSettings(Core::ICore::dialogParent()).value("Flavors").toMap();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        const QString flavor = it.key();
        if (flavor.isEmpty())
            continue;

        const QStringList osNames = it.value().toStringList();
        std::vector<Abi::OS> oses;
        for (const QString &osName : osNames) {
            Abi::OS os = Abi::osFromString(&osName);
            if (Abi::toString(os) != osName)
                qWarning() << "Invalid OS found when registering extra abi flavor" << it.key();
            else
                oses.push_back(os);
        }

        Abi::registerOsFlavor(oses, flavor);
    }
}

} // namespace Internal
} // namespace ProjectExplorer
