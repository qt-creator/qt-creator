// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../mesontools.h"

#include <utils/launcherinterface.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace MesonProjectManager::Internal;

static const QPair<const char *, QString> projectList[] = {{"Simple C Project", "simplecproject"}};

class AMesonWrapper : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        Utils::TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath()
                                                               + "/mesontest-XXXXXX");
        Utils::LauncherInterface::setPathToLauncher(qApp->applicationDirPath() + '/'
                                                    + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));

        const auto path = findTool(ToolType::Meson);
        if (!path)
            QSKIP("Meson not found");
    }

    void shouldFindMesonFromPATH()
    {
        const auto path = findTool(ToolType::Meson);
        QVERIFY(path);
        QVERIFY(path->exists());
    }

    void shouldReportMesonVersion()
    {
        ToolWrapper meson(ToolType::Meson, "name", *findTool(ToolType::Meson));
        QVERIFY(meson.isValid());
        QVERIFY(meson.version().major == 0);
        QVERIFY(meson.version().minor >= 50);
        QVERIFY(meson.version().minor <= 100);
        QVERIFY(meson.version().patch >= 0);
    }

    void shouldSetupGivenProjects_data()
    {
        QTest::addColumn<QString>("src_dir");
        for (const auto &project : projectList) {
            QTest::newRow(project.first)
                << QString("%1/%2").arg(MESON_SAMPLES_DIR).arg(project.second);
        }
    }

    void shouldSetupGivenProjects()
    {
        QFETCH(QString, src_dir);
        QTemporaryDir build_dir{"test-meson"};
        const ToolWrapper meson(ToolType::Meson, "name", *findTool(ToolType::Meson));
        QVERIFY(run_meson(meson.setup(Utils::FilePath::fromString(src_dir),
                                      Utils::FilePath::fromString(build_dir.path()))));
        QVERIFY(
            Utils::FilePath::fromString(build_dir.path() + "/meson-info/meson-info.json").exists());
        QVERIFY(isSetup(Utils::FilePath::fromString(build_dir.path())));
    }

    void shouldReConfigureGivenProjects_data()
    {
        QTest::addColumn<QString>("src_dir");
        for (const auto &project : projectList) {
            QTest::newRow(project.first)
                << QString("%1/%2").arg(MESON_SAMPLES_DIR).arg(project.second);
        }
    }

    void shouldReConfigureGivenProjects()
    {
        QFETCH(QString, src_dir);
        QTemporaryDir build_dir{"test-meson"};
        const ToolWrapper meson(ToolType::Meson, "name", *findTool(ToolType::Meson));
        QVERIFY(run_meson(meson.setup(Utils::FilePath::fromString(src_dir),
                                      Utils::FilePath::fromString(build_dir.path()))));
        QVERIFY(run_meson(meson.configure(Utils::FilePath::fromString(src_dir),
                                          Utils::FilePath::fromString(build_dir.path()))));
    }

    void cleanupTestCase()
    {
        Utils::Singleton::deleteAll();
    }
};

QTEST_GUILESS_MAIN(AMesonWrapper)
#include "testmesonwrapper.moc"
