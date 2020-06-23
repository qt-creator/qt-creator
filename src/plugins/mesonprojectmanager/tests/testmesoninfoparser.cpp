/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "exewrappers/mesonwrapper.h"
#include "mesoninfoparser/mesoninfoparser.h"
#include <iostream>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest/QtTest>

using namespace MesonProjectManager::Internal;

struct projectData
{
    const char *name;
    QString path;
    QStringList targets;
};

namespace {
static const QList<projectData> projectList{
    {"Simple C Project", "simplecproject", {"SimpleCProject"}}};
} // namespace

#define WITH_CONFIGURED_PROJECT(_source_dir, _build_dir, ...) \
    { \
        QTemporaryDir _build_dir{"test-meson"}; \
        const auto _meson = MesonWrapper("name", *MesonWrapper::find()); \
        run_meson(_meson.setup(Utils::FilePath::fromString(_source_dir), \
                               Utils::FilePath::fromString(_build_dir.path()))); \
        QVERIFY(isSetup(Utils::FilePath::fromString(_build_dir.path()))); \
        __VA_ARGS__ \
    }

#define WITH_UNCONFIGURED_PROJECT(_source_dir, _intro_file, ...) \
    { \
        QTemporaryFile _intro_file; \
        _intro_file.open(); \
        const auto _meson = MesonWrapper("name", *MesonWrapper::find()); \
        run_meson(_meson.introspect(Utils::FilePath::fromString(_source_dir)), &_intro_file); \
        __VA_ARGS__ \
    }

class AMesonInfoParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

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
        WITH_CONFIGURED_PROJECT(src_dir, build_dir, {
            auto result = MesonInfoParser::parse(build_dir.path());
            QStringList targetsNames;
            std::transform(std::cbegin(result.targets),
                           std::cend(result.targets),
                           std::back_inserter(targetsNames),
                           [](const auto &target) { return target.name; });
            QVERIFY(targetsNames == expectedTargets);
        })

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

    void cleanupTestCase() {}
};

QTEST_MAIN(AMesonInfoParser)
#include "testmesoninfoparser.moc"
