// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonwrapper.h"
#include "mesoninfoparser.h"

#include <utils/launcherinterface.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest/QtTest>

#include <iostream>

using namespace MesonProjectManager::Internal;
using namespace Utils;

struct projectData
{
    const char *name;
    QString path;
    QStringList targets;
};

static const QList<projectData> projectList{
    {"Simple C Project", "simplecproject", {"SimpleCProject"}}};


#define WITH_UNCONFIGURED_PROJECT(_source_dir, _intro_file, ...) \
    { \
        QTemporaryFile _intro_file; \
        _intro_file.open(); \
        const auto tool = MesonWrapper::find(); \
        QVERIFY(tool.has_value()); \
        const auto _meson = MesonWrapper("name", *tool); \
        run_meson(_meson.introspect(Utils::FilePath::fromString(_source_dir)), &_intro_file); \
        __VA_ARGS__ \
    }

class AMesonInfoParser : public QObject
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

    void shouldListTargets_data()
    {
        QTest::addColumn<QString>("src_dir");
        QTest::addColumn<QStringList>("expectedTargets");
        for (const auto &project : projectList) {
            QTest::newRow(project.name)
                << QString("%1/%2").arg(MESON_SAMPLES_DIR).arg(project.path) << project.targets;
        }
    }

    void shouldListTargets()
    {
        QFETCH(QString, src_dir);
        QFETCH(QStringList, expectedTargets);

        {
            QTemporaryDir build_dir{"test-meson"};
            FilePath buildDir = FilePath::fromString(build_dir.path());
            const auto tool = MesonWrapper::find();
            QVERIFY(tool.has_value());
            MesonWrapper meson("name", *tool);
            run_meson(meson.setup(FilePath::fromString(src_dir), buildDir));
            QVERIFY(isSetup(buildDir));

            auto result = MesonInfoParser::parse(buildDir);
            QStringList targetsNames;
            std::transform(std::cbegin(result.targets),
                           std::cend(result.targets),
                           std::back_inserter(targetsNames),
                           [](const auto &target) { return target.name; });
            QVERIFY(targetsNames == expectedTargets);
        }

        WITH_UNCONFIGURED_PROJECT(src_dir, introFile, {
            auto result = MesonInfoParser::parse(&introFile);
            QStringList targetsNames;
            std::transform(std::cbegin(result.targets),
                           std::cend(result.targets),
                           std::back_inserter(targetsNames),
                           [](const auto &target) { return target.name; });
            QVERIFY(targetsNames == expectedTargets);
        })
    }

    void cleanupTestCase()
    {
        Utils::Singleton::deleteAll();
    }

private:
};

QTEST_GUILESS_MAIN(AMesonInfoParser)
#include "testmesoninfoparser.moc"
