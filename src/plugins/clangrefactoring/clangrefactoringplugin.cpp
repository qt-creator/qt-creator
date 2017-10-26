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
#include "symbolquery.h"
#include "sqlitereadstatement.h"
#include "sqlitedatabase.h"
#include "querysqlitestatementfactory.h"

#include <clangpchmanager/qtcreatorprojectupdater.h>
#include <clangsupport/refactoringdatabaseinitializer.h>

#include <cpptools/cppmodelmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/find/searchresultwindow.h>
#include <extensionsystem/pluginmanager.h>

#include <refactoringdatabaseinitializer.h>
#include <filepathcaching.h>

#include <sqlitedatabase.h>

#include <utils/hostosinfo.h>

#include <QDir>
#include <QApplication>

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
    using ProjectUpdater = ClangPchManager::QtCreatorProjectUpdater<ClangPchManager::ProjectUpdater>;
public:
    using QuerySqliteReadStatementFactory = QuerySqliteStatementFactory<Sqlite::Database,
                                                                        Sqlite::ReadStatement>;

    Sqlite::Database database{Utils::PathString{QDir::tempPath() + "/symbol.db"}};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    RefactoringClient refactoringClient;
    ClangBackEnd::RefactoringConnectionClient connectionClient{&refactoringClient};
    QuerySqliteReadStatementFactory statementFactory{database};
    SymbolQuery<QuerySqliteReadStatementFactory> symbolQuery{statementFactory};
    RefactoringEngine engine{connectionClient.serverProxy(), refactoringClient, filePathCache, symbolQuery};

    QtCreatorSearch qtCreatorSearch{*Core::SearchResultWindow::instance()};
    QtCreatorClangQueryFindFilter qtCreatorfindFilter{connectionClient.serverProxy(),
                                                      qtCreatorSearch,
                                                      refactoringClient};
    ProjectUpdater projectUpdate{connectionClient.serverProxy()};
};

ClangRefactoringPlugin::ClangRefactoringPlugin()
{
}

ClangRefactoringPlugin::~ClangRefactoringPlugin()
{
}

bool ClangRefactoringPlugin::initialize(const QStringList & /*arguments*/, QString * /*errorMessage*/)
{
    d.reset(new ClangRefactoringPluginData);

    d->refactoringClient.setRefactoringEngine(&d->engine);
    d->refactoringClient.setRefactoringConnectionClient(&d->connectionClient);
    ExtensionSystem::PluginManager::addObject(&d->qtCreatorfindFilter);

    connectBackend();
    startBackend();

    return true;
}

void ClangRefactoringPlugin::extensionsInitialized()
{
    CppTools::CppModelManager::addRefactoringEngine(
                CppTools::RefactoringEngineType::ClangRefactoring, &refactoringEngine());
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

} // namespace ClangRefactoring
