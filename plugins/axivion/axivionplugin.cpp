// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "axivionplugin.h"

#include "axivionoutputpane.h"
#include "axivionsettings.h"
#include "axivionsettingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#ifdef LICENSECHECKER
#  include <licensechecker/licensecheckerplugin.h>
#endif

namespace Axivion::Internal {

class AxivionPluginPrivate
{
public:
    AxivionSettings axivionSettings;
    AxivionSettingsPage axivionSettingsPage{&axivionSettings};
    AxivionOutputPane axivionOutputPane;
};

AxivionPlugin::~AxivionPlugin()
{
    delete d;
}

bool AxivionPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

#ifdef LICENSECHECKER
    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (!licenseChecker || !licenseChecker->hasValidLicense() || !licenseChecker->enterpriseFeatures())
        return true;
#endif // LICENSECHECKER

    d = new AxivionPluginPrivate;
    d->axivionSettings.fromSettings(Core::ICore::settings());
    return true;
}

} // Axivion::Internal
