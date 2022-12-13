// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "axivionplugin.h"

#include "axivionoutputpane.h"
#include "axivionprojectsettings.h"
#include "axivionsettings.h"
#include "axivionsettingspage.h"
#include "axiviontr.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <utils/qtcassert.h>

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
    QHash<ProjectExplorer::Project *, AxivionProjectSettings *> projectSettings;
};

static AxivionPluginPrivate *dd = nullptr;

AxivionPlugin::~AxivionPlugin()
{
    if (!dd->projectSettings.isEmpty()) {
        qDeleteAll(dd->projectSettings);
        dd->projectSettings.clear();
    }
    delete dd;
    dd = nullptr;
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

    dd = new AxivionPluginPrivate;
    dd->axivionSettings.fromSettings(Core::ICore::settings());

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory;
    panelFactory->setPriority(250);
    panelFactory->setDisplayName(Tr::tr("Axivion"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project){
        return new AxivionProjectSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
    return true;
}

AxivionSettings *AxivionPlugin::settings()
{
    QTC_ASSERT(dd, return nullptr);
    return &dd->axivionSettings;
}

AxivionProjectSettings *AxivionPlugin::projectSettings(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return nullptr);
    QTC_ASSERT(dd, return nullptr);

    auto &settings = dd->projectSettings[project];
    if (!settings)
        settings = new AxivionProjectSettings(project);
    return settings;
}

} // Axivion::Internal
