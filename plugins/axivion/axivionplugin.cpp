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

#include <QMessageBox>

namespace Axivion::Internal {

class AxivionPluginPrivate
{
public:
    AxivionSettings axivionSettings;
    AxivionSettingsPage axivionSettingsPage{&axivionSettings};
    AxivionOutputPane axivionOutputPane;
    QHash<ProjectExplorer::Project *, AxivionProjectSettings *> projectSettings;
};

static AxivionPlugin *s_instance = nullptr;
static AxivionPluginPrivate *dd = nullptr;

AxivionPlugin::AxivionPlugin()
{
    s_instance = this;
}

AxivionPlugin::~AxivionPlugin()
{
    if (!dd->projectSettings.isEmpty()) {
        qDeleteAll(dd->projectSettings);
        dd->projectSettings.clear();
    }
    delete dd;
    dd = nullptr;
}

AxivionPlugin *AxivionPlugin::instance()
{
    return s_instance;
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

bool AxivionPlugin::handleCertificateIssue()
{
    QTC_ASSERT(dd, return false);

    const QString serverHost = QUrl(dd->axivionSettings.server.dashboard).host();
    if (QMessageBox::question(Core::ICore::dialogParent(), Tr::tr("Certificate Error"),
                              Tr::tr("Server certificate for %1 cannot be authenticated.\n"
                                     "Do you want to disable SSL verification for this server?\n"
                                     "Note: This can expose you to man-in-the-middle attack.")
                              .arg(serverHost))
            != QMessageBox::Yes) {
        return false;
    }
    dd->axivionSettings.server.validateCert = false;
    emit s_instance->settingsChanged();
    return true;
}

} // Axivion::Internal
