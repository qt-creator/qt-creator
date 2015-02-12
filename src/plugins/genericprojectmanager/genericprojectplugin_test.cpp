/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "genericprojectplugin.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolstestcase.h>

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

static QStringList simplify(const QList<ProjectFile> &files, const QString &prefix)
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

    QList<ProjectPart::Ptr> parts = pInfo.projectParts();
    std::sort(parts.begin(), parts.end(), [](const ProjectPart::Ptr &p1,
                                             const ProjectPart::Ptr &p2) {
        return p1->displayName < p2->displayName;
    });

    const QString dirPathWithSlash = temporaryDir.path() + QLatin1Char('/');
    const QStringList part0files = simplify(parts[0]->files, dirPathWithSlash);
    const QStringList part1files = simplify(parts[1]->files, dirPathWithSlash);
    const QStringList part2files = simplify(parts[2]->files, dirPathWithSlash);

    QCOMPARE(parts[0]->displayName, _("mixedproject1 (C++11)"));
    QCOMPARE(parts[0]->files.size(), 4);
    QVERIFY(part0files.contains(_("main.cpp")));
    QVERIFY(part0files.contains(_("header.h")));
    QVERIFY(part0files.contains(_("MyViewController.h")));
    QVERIFY(part0files.contains(_("Glue.h")));

    QCOMPARE(parts[1]->displayName, _("mixedproject1 (Obj-C++11)"));
    QCOMPARE(parts[1]->files.size(), 4);
    QVERIFY(part1files.contains(_("Glue.mm")));
    QVERIFY(part1files.contains(_("header.h")));
    QVERIFY(part1files.contains(_("MyViewController.h")));
    QVERIFY(part1files.contains(_("Glue.h")));

    QCOMPARE(parts[2]->displayName, _("mixedproject1 (Obj-C11)"));
    QCOMPARE(parts[2]->files.size(), 1);
    QVERIFY(part2files.contains(_("MyViewController.m")));
    // No .h files here, because the mime-type for .h files is.....
    //
    // wait for it...
    //
    // C++!
    // (See 1c7da3d83c9bb35064ae6b9052cbf1c6bff1395e.)
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

    QList<ProjectPart::Ptr> parts = pInfo.projectParts();
    std::sort(parts.begin(), parts.end(), [](const ProjectPart::Ptr &p1,
                                             const ProjectPart::Ptr &p2) {
        return p1->displayName < p2->displayName;
    });

    const QString dirPathWithSlash = temporaryDir.path() + QLatin1Char('/');
    const QStringList part0files = simplify(parts[0]->files, dirPathWithSlash);
    const QStringList part1files = simplify(parts[1]->files, dirPathWithSlash);

    QCOMPARE(parts[0]->displayName, _("mixedproject2 (C++11)"));
    QCOMPARE(parts[0]->files.size(), 2);
    QVERIFY(part0files.contains(_("main.cpp")));
    QVERIFY(part0files.contains(_("header.hpp")));

    QCOMPARE(parts[1]->displayName, _("mixedproject2 (C11)"));
    QCOMPARE(parts[1]->files.size(), 1);
    QVERIFY(part1files.contains(_("impl.c")));
}
