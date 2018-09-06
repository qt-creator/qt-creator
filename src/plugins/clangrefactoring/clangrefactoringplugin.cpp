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
#include <clangsupport/refactoringdatabaseinitializer.h>

#include <cpptools/cppmodelmanager.h>

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/cpptoolsconstants.h>

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
    Sqlite::Database database{Utils::PathString{Core::ICore::userResourcePath() + "/symbol-experimental-v1.db"}, 1000ms};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    RefactoringClient refactoringClient;
    QtCreatorEditorManager editorManager{filePathCache};
    ClangBackEnd::RefactoringConnectionClient connectionClient{&refactoringClient};
    QuerySqliteReadStatementFactory statementFactory{database};
    SymbolQuery<QuerySqliteReadStatementFactory> symbolQuery{statementFactory};
    RefactoringEngine engine{connectionClient.serverProxy(), refactoringClient, filePathCache, symbolQuery};

    QtCreatorSearch qtCreatorSearch;
    QtCreatorClangQueryFindFilter qtCreatorfindFilter{connectionClient.serverProxy(),
                                                      qtCreatorSearch,
                                                      refactoringClient};
    QtCreatorRefactoringProjectUpdater projectUpdate{connectionClient.serverProxy(),
                                                     ClangPchManager::ClangPchManagerPlugin::pchManagerClient(),
                                                     filePathCache};
};

ClangRefactoringPlugin::ClangRefactoringPlugin()
{
}

ClangRefactoringPlugin::~ClangRefactoringPlugin()
{
}

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
    ExtensionSystem::PluginManager::addObject(&d->qtCreatorfindFilter);

    connectBackend();
    startBackend();

    CppTools::CppModelManager::addRefactoringEngine(
                CppTools::RefactoringEngineType::ClangRefactoring, &refactoringEngine());

    initializeFilters();

    return true;
}

void ClangRefactoringPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag ClangRefactoringPlugin::aboutToShutdown()
{
    ExtensionSystem::PluginManager::removeObject(&d->qtCreatorfindFilter);
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
