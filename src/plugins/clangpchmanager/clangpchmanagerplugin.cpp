/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangpchmanagerplugin.h"

#include "clangindexingprojectsettingswidget.h"
#include "clangindexingsettingsmanager.h"
#include "pchmanagerclient.h"
#include "pchmanagerconnectionclient.h"
#include "progressmanager.h"
#include "qtcreatorprojectupdater.h"

#include <filepathcaching.h>
#include <projectpartsstorage.h>
#include <refactoringdatabaseinitializer.h>
#include <sqlitedatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectpanelfactory.h>

#include <utils/hostosinfo.h>

#include <QFutureInterface>

#include <chrono>
#include <map>

using namespace std::chrono_literals;

namespace ClangPchManager {

namespace {

QString backendProcessPath()
{
    return Core::ICore::libexecPath()
            + QStringLiteral("/clangpchmanagerbackend")
            + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

void addIndexingProjectPaneWidget(ClangIndexingSettingsManager &settingsManager,
                                  QtCreatorProjectUpdater<PchManagerProjectUpdater> &projectUpdater)
{
    auto factory = new ProjectExplorer::ProjectPanelFactory;
    factory->setPriority(120);
    factory->setDisplayName(ClangIndexingProjectSettingsWidget::tr("Clang Indexing"));
    factory->setCreateWidgetFunction([&](ProjectExplorer::Project *project) {
        auto widget = new ClangIndexingProjectSettingsWidget(settingsManager.settings(project),
                                                             project,
                                                             projectUpdater);

        widget->onProjectPartsUpdated(project);

        QObject::connect(CppTools::CppModelManager::instance(),
                         &CppTools::CppModelManager::projectPartsUpdated,
                         widget,
                         &ClangIndexingProjectSettingsWidget::onProjectPartsUpdated);

        return widget;
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(factory);
}

} // anonymous namespace

class ClangPchManagerPluginData
{
public:
    Sqlite::Database database{Utils::PathString{Core::ICore::cacheResourcePath()
                                                + "/symbol-experimental-v1.db"},
                              1000ms};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangPchManager::ProgressManager pchCreationProgressManager{[](QFutureInterface<void> &promise) {
        auto title = QCoreApplication::translate("ClangPchProgressManager",
                                                 "Creating PCHs",
                                                 "PCH stands for precompiled header");
        Core::ProgressManager::addTask(promise.future(), title, "pch creation", {});
    }};
    ClangPchManager::ProgressManager dependencyCreationProgressManager{
        [](QFutureInterface<void> &promise) {
            auto title = QCoreApplication::translate("ClangPchProgressManager",
                                                     "Creating Dependencies");
            Core::ProgressManager::addTask(promise.future(), title, "dependency creation", {});
        }};
    ClangBackEnd::ProjectPartsStorage<Sqlite::Database> projectPartsStorage{database};
    PchManagerClient pchManagerClient{pchCreationProgressManager, dependencyCreationProgressManager};
    PchManagerConnectionClient connectionClient{&pchManagerClient};
    ClangIndexingSettingsManager settingsManager;
    QtCreatorProjectUpdater<PchManagerProjectUpdater> projectUpdater{connectionClient.serverProxy(),
                                                                     pchManagerClient,
                                                                     filePathCache,
                                                                     projectPartsStorage,
                                                                     settingsManager};
};

std::unique_ptr<ClangPchManagerPluginData> ClangPchManagerPlugin::d;

ClangPchManagerPlugin::ClangPchManagerPlugin() = default;
ClangPchManagerPlugin::~ClangPchManagerPlugin() = default;

bool ClangPchManagerPlugin::initialize(const QStringList & /*arguments*/, QString * /*errorMessage*/)
{
    QDir{}.mkpath(Core::ICore::cacheResourcePath());

    d = std::make_unique<ClangPchManagerPluginData>();

    startBackend();

    addIndexingProjectPaneWidget(d->settingsManager, d->projectUpdater);

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag ClangPchManagerPlugin::aboutToShutdown()
{
    d->connectionClient.finishProcess();

    d.reset();

    return SynchronousShutdown;
}

void ClangPchManagerPlugin::startBackend()
{
    d->pchManagerClient.setConnectionClient(&d->connectionClient);

    d->connectionClient.setProcessPath(backendProcessPath());

    d->connectionClient.startProcessAndConnectToServerAsynchronously();
}


PchManagerClient &ClangPchManagerPlugin::pchManagerClient()
{
    return d->pchManagerClient;
}

ClangIndexingSettingsManager &ClangPchManagerPlugin::settingsManager()
{
    return d->settingsManager;
}

} // namespace ClangRefactoring
