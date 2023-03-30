// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/iplugin.h>

#include <utils/predicates.h>
#include <utils/algorithm.h>

#include <QMetaObject>

namespace QmlDesigner {

enum FoundLicense {
    noLicense,
    community,
    professional,
    enterprise
};

namespace Internal {
inline ExtensionSystem::IPlugin *licenseCheckerPlugin()
{
    const ExtensionSystem::PluginSpec *pluginSpec = Utils::findOrDefault(
        ExtensionSystem::PluginManager::plugins(),
        Utils::equal(&ExtensionSystem::PluginSpec::name, QString("LicenseChecker")));

    if (pluginSpec)
        return pluginSpec->plugin();
    return nullptr;
}

inline ExtensionSystem::IPlugin *dsLicenseCheckerPlugin()
{
    const ExtensionSystem::PluginSpec *pluginSpec = Utils::findOrDefault(
        ExtensionSystem::PluginManager::plugins(),
        Utils::equal(&ExtensionSystem::PluginSpec::name, QString("DSLicense")));

    if (pluginSpec)
        return pluginSpec->plugin();
    return nullptr;
}

inline bool dsLicenseCheckerPluginExists()
{
    const ExtensionSystem::PluginSpec *pluginSpec = Utils::findOrDefault(
        ExtensionSystem::PluginManager::plugins(),
        Utils::equal(&ExtensionSystem::PluginSpec::name, QString("DSLicense")));

    return pluginSpec;
}
} // namespace Internal

inline FoundLicense checkLicense()
{
    static FoundLicense license = noLicense;

    if (license != noLicense)
        return license;

    if (auto plugin = Internal::licenseCheckerPlugin()) {
        bool retVal = false;

        bool success = QMetaObject::invokeMethod(plugin,
                                                 "evaluationLicense",
                                                 Qt::DirectConnection,
                                                 Q_RETURN_ARG(bool, retVal));

        if (success && retVal)
            return enterprise;

        retVal = false;

        success = QMetaObject::invokeMethod(plugin,
                                            "qdsEnterpriseLicense",
                                            Qt::DirectConnection,
                                            Q_RETURN_ARG(bool, retVal));
        if (success && retVal)
            return enterprise;
        else
            return professional;
    }
    return community;
}

inline QString licensee()
{
    if (auto plugin = Internal::licenseCheckerPlugin()) {
        QString retVal;
        bool success = QMetaObject::invokeMethod(plugin,
                                                 "licensee",
                                                 Qt::DirectConnection,
                                                 Q_RETURN_ARG(QString, retVal));
        if (success)
            return retVal;
    }
    return {};
}

inline QString licenseeEmail()
{
    if (auto plugin = Internal::licenseCheckerPlugin()) {
        QString retVal;
        bool success = QMetaObject::invokeMethod(plugin,
                                                 "licenseeEmail",
                                                 Qt::DirectConnection,
                                                 Q_RETURN_ARG(QString, retVal));
        if (success)
            return retVal;
    }
    return {};
}

inline bool checkEnterpriseLicense()
{
    if (auto plugin = Internal::dsLicenseCheckerPlugin()) {
        bool retVal = false;
        bool success = QMetaObject::invokeMethod(plugin,
                                                 "checkEnterpriseLicense",
                                                 Qt::DirectConnection,
                                                 Q_RETURN_ARG(bool, retVal));

        if (success)
            return retVal;
    }

    if (Internal::dsLicenseCheckerPluginExists())
        return false;

    return true;
}

} // namespace Utils
