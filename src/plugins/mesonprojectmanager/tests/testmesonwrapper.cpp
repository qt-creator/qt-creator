// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../mesonwrapper.h"

#include <utils/launcherinterface.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <iostream>

using namespace MesonProjectManager::Internal;

namespace {
static const QList<QPair<const char *, QString>> projectList{{"Simple C Project", "simplecproject"}};
} // namespace

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

        const auto path = MesonWrapper::find();
        if (!path)
            QSKIP("Meson not found");
    }

    void shouldFindMesonFromPATH()
    {
        const auto path = MesonWrapper::find();
        QVERIFY(path);
        QVERIFY(path->exists());
    }

    void shouldReportMesonVersion()
    {
        const auto meson = MesonWrapper("name", *MesonWrapper::find());
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
        const auto meson = MesonWrapper("name", *MesonWrapper::find());
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
        const auto meson = MesonWrapper("name", *MesonWrapper::find());
        QVERIFY(run_meson(meson.setup(Utils::FilePath::fromString(src_dir),
                                      Utils::FilePath::fromString(build_dir.path()))));
        QVERIFY(run_meson(meson.configure(Utils::FilePath::fromString(src_dir),
                                          Utils::FilePath::fromString(build_dir.path()))));
    }

    void cleanupTestCase()
    {
        Utils::Singleton::deleteAll();
    }

private:
};

QTEST_GUILESS_MAIN(AMesonWrapper)
#include "testmesonwrapper.moc"
