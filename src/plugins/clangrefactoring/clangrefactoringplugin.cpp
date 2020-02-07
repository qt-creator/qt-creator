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

#include "clangrefactoringplugin.h"
#include "locatorfilter.h"
#include "qtcreatorsymbolsfindfilter.h"
#include "qtcreatoreditormanager.h"
#include "qtcreatorrefactoringprojectupdater.h"
#include "querysqlitestatementfactory.h"
#include "sqlitedatabase.h"
#include "sqlitereadstatement.h"
#include "symbolquery.h"

#include <clangpchmanager/clangpchmanagerplugin.h>
#include <clangpchmanager/progressmanager.h>
#include <clangsupport/refactoringdatabaseinitializer.h>
#include <projectpartsstorage.h>

#include <cpptools/cppmodelmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <extensionsystem/pluginmanager.h>

#include <refactoringdatabaseinitializer.h>
#include <filepathcaching.h>

#include <sqlitedatabase.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QApplication>

#include <chrono>

using namespace std::chrono_literals;

namespace ClangRefactoring {

namespace {

using ClangPchManager::ClangPchManagerPlugin;

QString backendProcessPath()
{
    return Core::ICore::libexecPath()
            + QStringLiteral("/clangrefactoringbackend")
            + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

} // anonymous namespace

std::unique_ptr<ClangRefactoringPluginData> ClangRefactoringPlugin::d;

class ClangRefactoringPluginData
{
public:
    using QuerySqliteReadStatementFactory = QuerySqliteStatementFactory<Sqlite::Database,
                                                                        Sqlite::ReadStatement>;
    Sqlite::Database database{Utils::PathString{Core::ICore::cacheResourcePath()
                                                + "/symbol-experimental-v1.db"},
                              1000ms};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangPchManager::ProgressManager progressManager{
        [] (QFutureInterface<void> &promise) {
            auto title = QCoreApplication::translate("ClangRefactoringProgressManager", "C++ Indexing");
            Core::ProgressManager::addTask(promise.future(), title, "clang indexing", {});}};
    RefactoringClient refactoringClient{progressManager};
    QtCreatorEditorManager editorManager{filePathCache};
    ClangBackEnd::RefactoringConnectionClient connectionClient{&refactoringClient};
    QuerySqliteReadStatementFactory statementFactory{database};
    SymbolQuery<QuerySqliteReadStatementFactory> symbolQuery{statementFactory};
    ClangBackEnd::ProjectPartsStorage<Sqlite::Database> projectPartsStorage{database};
    RefactoringEngine engine{connectionClient.serverProxy(), refactoringClient, filePathCache, symbolQuery};
    QtCreatorRefactoringProjectUpdater projectUpdate{connectionClient.serverProxy(),
                                                     ClangPchManagerPlugin::pchManagerClient(),
                                                     filePathCache,
                                                     projectPartsStorage,
                                                     ClangPchManagerPlugin::settingsManager()};
};

ClangRefactoringPlugin::ClangRefactoringPlugin() = default;

ClangRefactoringPlugin::~ClangRefactoringPlugin() = default;

static bool useClangFilters()
{
    static bool use = qEnvironmentVariableIntValue("QTC_CLANG_LOCATORS");
    return use;
}

bool ClangRefactoringPlugin::initialize(const QStringList & /*arguments*/, QString * /*errorMessage*/)
{
    d = std::make_unique<ClangRefactoringPluginData>();

    d->refactoringClient.setRefactoringEngine(&d->engine);
    d->refactoringClient.setRefactoringConnectionClient(&d->connectionClient);

    connectBackend();
    startBackend();

    CppTools::CppModelManager::addRefactoringEngine(CppTools::RefactoringEngineType::ClangRefactoring,
                                                    &refactoringEngine());

    initializeFilters();

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag ClangRefactoringPlugin::aboutToShutdown()
{
    CppTools::CppModelManager::removeRefactoringEngine(
                CppTools::RefactoringEngineType::ClangRefactoring);
    d->refactoringClient.setRefactoringConnectionClient(nullptr);
    d->refactoringClient.setRefactoringEngine(nullptr);

    d.reset();

    return SynchronousShutdown;
}

RefactoringEngine &ClangRefactoringPlugin::refactoringEngine()
{
    return d->engine;
}

void ClangRefactoringPlugin::startBackend()
{
    d->connectionClient.setProcessPath(backendProcessPath());

    d->connectionClient.startProcessAndConnectToServerAsynchronously();
}

void ClangRefactoringPlugin::connectBackend()
{
    connect(&d->connectionClient,
            &ClangBackEnd::RefactoringConnectionClient::connectedToLocalSocket,
            this,
            &ClangRefactoringPlugin::backendIsConnected);
}

void ClangRefactoringPlugin::backendIsConnected()
{
    d->engine.setRefactoringEngineAvailable(true);
}

void ClangRefactoringPlugin::initializeFilters()
{
    if (!useClangFilters())
        return;

    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    modelManager->setClassesFilter(std::make_unique<LocatorFilter>(
                                       d->symbolQuery,
                                       d->editorManager,
                                       ClangBackEnd::SymbolKinds{ClangBackEnd::SymbolKind::Record},
                                       CppTools::Constants::CLASSES_FILTER_ID,
                                       CppTools::Constants::CLASSES_FILTER_DISPLAY_NAME,
                                       "c"));
    modelManager->setFunctionsFilter(std::make_unique<LocatorFilter>(
                                       d->symbolQuery,
                                       d->editorManager,
                                       ClangBackEnd::SymbolKinds{ClangBackEnd::SymbolKind::Function},
                                       CppTools::Constants::FUNCTIONS_FILTER_ID,
                                       CppTools::Constants::FUNCTIONS_FILTER_DISPLAY_NAME,
                                       "m"));
    modelManager->setLocatorFilter(std::make_unique<LocatorFilter>(
                                         d->symbolQuery,
                                         d->editorManager,
                                         ClangBackEnd::SymbolKinds{ClangBackEnd::SymbolKind::Record,
                                                     ClangBackEnd::SymbolKind::Enumeration,
                                                     ClangBackEnd::SymbolKind::Function},
                                         CppTools::Constants::LOCATOR_FILTER_ID,
                                         CppTools::Constants::LOCATOR_FILTER_DISPLAY_NAME,
                                         ":"));
}

} // namespace ClangRefactoring
