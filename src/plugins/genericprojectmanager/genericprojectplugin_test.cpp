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

#include "genericprojectplugin.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>
#include <cpptools/projectinfo.h>

#include <QFileInfo>
#include <QTest>

using namespace CppTools;
using namespace CppTools::Tests;
using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;
using namespace ProjectExplorer;

namespace {

inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }
inline QString sourceProjectPath(const QString &project)
{
    const QString fileName(_(SRCDIR "/../../../tests/genericprojectmanager/") + project);
    return QFileInfo(fileName).absoluteFilePath();
}

} // anonymous namespace

void GenericProjectPlugin::test_simple()
{
    Tests::VerifyCleanCppModelManager verify;

    TemporaryCopiedDir temporaryDir(sourceProjectPath(_("testdata_simpleproject")));
    QVERIFY(temporaryDir.isValid());
    const QString mainFile = temporaryDir.absolutePath("main.cpp");
    const QString projectFile = temporaryDir.absolutePath("simpleproject.creator");

    ProjectOpenerAndCloser projects;
    const ProjectInfo pInfo = projects.open(projectFile);
    QVERIFY(pInfo.isValid());
    QCOMPARE(pInfo.projectParts().size(), 1);

    ProjectPart::Ptr pPart = pInfo.projectParts().first();
    QVERIFY(pPart);
    QCOMPARE(pPart->files.size(), 1);
    QCOMPARE(pPart->files.first().path, mainFile);
    QCOMPARE(pPart->files.first().kind, ProjectFile::CXXSource);
}

static QStringList simplify(const ProjectFiles &files, const QString &prefix)
{
    QStringList result;

    foreach (const ProjectFile &file, files) {
        if (file.path.startsWith(prefix))
            result.append(file.path.mid(prefix.size()));
        else
            result.append(file.path);
    }

    return result;
}

void GenericProjectPlugin::test_mixed1()
{
    Tests::VerifyCleanCppModelManager verify;

    TemporaryCopiedDir temporaryDir(sourceProjectPath(_("testdata_mixedproject1/")));
    QVERIFY(temporaryDir.isValid());
    const QString projectFile = temporaryDir.absolutePath("mixedproject1.creator");

    ProjectOpenerAndCloser projects;
    const ProjectInfo pInfo = projects.open(projectFile);
    QVERIFY(pInfo.isValid());
    QCOMPARE(pInfo.projectParts().size(), 3);

    QVector<ProjectPart::Ptr> parts = pInfo.projectParts();
    std::sort(parts.begin(), parts.end(), [](const ProjectPart::Ptr &p1,
                                             const ProjectPart::Ptr &p2) {
        return p1->displayName < p2->displayName;
    });

    const QString dirPathWithSlash = temporaryDir.path() + QLatin1Char('/');
    const QStringList part0files = simplify(parts[0]->files, dirPathWithSlash);
    const QStringList part1files = simplify(parts[1]->files, dirPathWithSlash);
    const QStringList part2files = simplify(parts[2]->files, dirPathWithSlash);

    QCOMPARE(parts[0]->displayName, _("mixedproject1 (C++)"));
    QCOMPARE(parts[0]->files.size(), 4);
    QVERIFY(part0files.contains(_("main.cpp")));
    QVERIFY(part0files.contains(_("header.h")));
    QVERIFY(part0files.contains(_("MyViewController.h")));
    QVERIFY(part0files.contains(_("Glue.h")));

    QCOMPARE(parts[1]->displayName, _("mixedproject1 (Obj-C)"));
    QCOMPARE(parts[1]->files.size(), 4);
    QVERIFY(part1files.contains(_("MyViewController.m")));
    QVERIFY(part1files.contains(_("header.h")));
    QVERIFY(part1files.contains(_("MyViewController.h")));
    QVERIFY(part1files.contains(_("Glue.h")));

    QCOMPARE(parts[2]->displayName, _("mixedproject1 (Obj-C++)"));
    QCOMPARE(parts[2]->files.size(), 4);
    QVERIFY(part2files.contains(_("Glue.mm")));
    QVERIFY(part2files.contains(_("header.h")));
    QVERIFY(part2files.contains(_("MyViewController.h")));
    QVERIFY(part2files.contains(_("Glue.h")));
}

void GenericProjectPlugin::test_mixed2()
{
    Tests::VerifyCleanCppModelManager verify;

    TemporaryCopiedDir temporaryDir(sourceProjectPath(_("testdata_mixedproject2/")));
    QVERIFY(temporaryDir.isValid());
    const QString projectFile = temporaryDir.absolutePath("mixedproject2.creator");

    ProjectOpenerAndCloser projects;
    const ProjectInfo pInfo = projects.open(projectFile);
    QVERIFY(pInfo.isValid());
    QCOMPARE(pInfo.projectParts().size(), 2);

    QVector<ProjectPart::Ptr> parts = pInfo.projectParts();
    std::sort(parts.begin(), parts.end(), [](const ProjectPart::Ptr &p1,
                                             const ProjectPart::Ptr &p2) {
        return p1->displayName < p2->displayName;
    });

    const QString dirPathWithSlash = temporaryDir.path() + QLatin1Char('/');
    const QStringList part0files = simplify(parts[0]->files, dirPathWithSlash);
    const QStringList part1files = simplify(parts[1]->files, dirPathWithSlash);

    QCOMPARE(parts[0]->displayName, _("mixedproject2 (C)"));
    QCOMPARE(parts[0]->files.size(), 1);
    QVERIFY(part0files.contains(_("impl.c")));

    QCOMPARE(parts[1]->displayName, _("mixedproject2 (C++)"));
    QCOMPARE(parts[1]->files.size(), 2);
    QVERIFY(part1files.contains(_("main.cpp")));
    QVERIFY(part1files.contains(_("header.hpp")));
}
