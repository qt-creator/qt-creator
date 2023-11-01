// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extraabi.h"

#include "abi.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/settingsaccessor.h>
#include <utils/settingsaccessor.h>

#include <QDebug>
#include <QGuiApplication>

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

    Store upgrade(const Store &data) override { return data; }
};

class AbiFlavorAccessor : public UpgradingSettingsAccessor
{
public:
    AbiFlavorAccessor()
    {
        setDocType("QtCreatorExtraAbi");
        setApplicationDisplayName(QGuiApplication::applicationDisplayName());
        setBaseFilePath(Core::ICore::installerResourcePath("abi.xml"));
        addVersionUpgrader(std::make_unique<AbiFlavorUpgraderV0>());
    }
};


// --------------------------------------------------------------------
// ExtraAbi:
// --------------------------------------------------------------------

void ExtraAbi::load()
{
    AbiFlavorAccessor accessor;
    const Store data = storeFromVariant(accessor.restoreSettings(Core::ICore::dialogParent()).value("Flavors"));
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        const Key flavor = it.key();
        if (flavor.isEmpty())
            continue;

        const QStringList osNames = it.value().toStringList();
        std::vector<Abi::OS> oses;
        for (const QString &osName : osNames) {
            Abi::OS os = Abi::osFromString(osName);
            if (Abi::toString(os) != osName) {
                qWarning() << "Invalid OS found when registering extra abi flavor"
                           << it.key().toByteArray();
            } else {
                oses.push_back(os);
            }
        }

        Abi::registerOsFlavor(oses, stringFromKey(flavor));
    }
}

} // namespace Internal
} // namespace ProjectExplorer
