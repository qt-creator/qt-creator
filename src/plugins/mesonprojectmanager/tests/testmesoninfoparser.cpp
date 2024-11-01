// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../mesoninfoparser.h"
#include "../mesontools.h"

#include <utils/processinterface.h>
#include <utils/singleton.h>
#include <utils/temporarydirectory.h>

#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest/QtTest>

using namespace MesonProjectManager::Internal;
using namespace Utils;

struct ProjectData
{
    const char *name;
    QString path;
    QStringList targets;
};

static const ProjectData projectList[] =
    {{"Simple C Project", "simplecproject", {"SimpleCProject"}}};


class AMesonInfoParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        Utils::TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath()
                                                               + "/mesontest-XXXXXX");

        const auto path = findTool(ToolType::Meson);
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
            const auto tool = findTool(ToolType::Meson);
            QVERIFY(tool.has_value());
            ToolWrapper meson(ToolType::Meson, "name", *tool);
            run_meson(meson.setup(FilePath::fromString(src_dir), buildDir));
            QVERIFY(isSetup(buildDir));

            MesonInfoParser::Result result = MesonInfoParser::parse(buildDir);

            QStringList targetsNames;
            std::transform(std::cbegin(result.targets),
                           std::cend(result.targets),
                           std::back_inserter(targetsNames),
                           [](const auto &target) { return target.name; });
            QVERIFY(targetsNames == expectedTargets);
        }

        {
            // With unconfigured project
            QTemporaryFile introFile;
            introFile.open();
            const auto tool = findTool(ToolType::Meson);
            QVERIFY(tool.has_value());
            const ToolWrapper meson(ToolType::Meson, "name", *tool);
            run_meson(meson.introspect(Utils::FilePath::fromString(src_dir)), &introFile);

            MesonInfoParser::Result result = MesonInfoParser::parse(&introFile);

            QStringList targetsNames;
            std::transform(std::cbegin(result.targets),
                           std::cend(result.targets),
                           std::back_inserter(targetsNames),
                           [](const auto &target) { return target.name; });
            QVERIFY(targetsNames == expectedTargets);
        }
    }

    void cleanupTestCase()
    {
        Utils::Singleton::deleteAll();
    }
};

QTEST_GUILESS_MAIN(AMesonInfoParser)
#include "testmesoninfoparser.moc"
