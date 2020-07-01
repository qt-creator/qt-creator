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
#include <iostream>
#include <QDir>
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace MesonProjectManager::Internal;

namespace {
static const QList<QPair<const char *, QString>> projectList{{"Simple C Project", "simplecproject"}};
} // namespace

class AMesonWrapper : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

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

    void cleanupTestCase() {}
};

QTEST_MAIN(AMesonWrapper)
#include "testmesonwrapper.moc"
