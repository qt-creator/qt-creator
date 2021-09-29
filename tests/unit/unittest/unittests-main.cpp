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

#include "googletest.h"

#include <sqlitedatabase.h>
#include <sqlitelibraryinitializer.h>

#include <sqliteglobal.h>
#include <utils/launcherinterface.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QScopeGuard>

#ifdef WITH_BENCHMARKS
#include <benchmark/benchmark.h>
#endif

class Environment : public testing::Environment
{
public:
    void SetUp() override
    {
        const QString temporayDirectoryPath = QDir::tempPath() + "/QtCreator-UnitTests-XXXXXX";
        Utils::TemporaryDirectory::setMasterTemporaryDirectory(temporayDirectoryPath);
        qputenv("TMPDIR", Utils::TemporaryDirectory::masterDirectoryPath().toUtf8());
        qputenv("TEMP", Utils::TemporaryDirectory::masterDirectoryPath().toUtf8());
    }

    void TearDown() override {}
};

int main(int argc, char *argv[])
{
    Sqlite::LibraryInitializer::initialize();
    Sqlite::Database::activateLogging();

    QGuiApplication application(argc, argv);
    Utils::LauncherInterface::setPathToLauncher(qApp->applicationDirPath() + '/'
                                                + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
    testing::InitGoogleTest(&argc, argv);
#ifdef WITH_BENCHMARKS
    benchmark::Initialize(&argc, argv);
#endif

    auto environment = std::make_unique<Environment>();
    testing::AddGlobalTestEnvironment(environment.release());

    int testsHaveErrors = RUN_ALL_TESTS();

    Utils::Singleton::deleteAll();
#ifdef WITH_BENCHMARKS
    if (testsHaveErrors == 0  && application.arguments().contains(QStringLiteral("--with-benchmarks")))
        benchmark::RunSpecifiedBenchmarks();
#endif

    return testsHaveErrors;
}
