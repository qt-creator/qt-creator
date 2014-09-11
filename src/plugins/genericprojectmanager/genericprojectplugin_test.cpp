/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppmodelmanagerhelper.h"
#include "genericprojectplugin.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QFileInfo>
#include <QTest>

using namespace CppTools;
using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;
using namespace GenericProjectManager::Internal::Tests;
using namespace ProjectExplorer;

inline static QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }
inline static QString projectFilePath(const QString &project)
{
    const QString fileName(_(SRCDIR "/../../../tests/genericprojectmanager/") + project);
    return QFileInfo(fileName).absoluteFilePath();
}

namespace {
class ProjectExplorerHelper
{
public:
    ProjectExplorerHelper()
    {
        QVERIFY(!SessionManager::hasProjects());
    }

    ~ProjectExplorerHelper()
    {
        foreach (Project *project, m_openProjects)
            ProjectExplorerPlugin::unloadProject(project);
    }

    Project *openProject(const QString &projectFile)
    {
        QString error;
        Project *project = ProjectExplorerPlugin::openProject(projectFile, &error);
        if (!error.isEmpty())
            qWarning() << error;
        if (!project)
            return 0;
        m_openProjects.append(project);
        return project;
    }

private:
    QList<Project *> m_openProjects;
};
} // anonymous namespace

static ProjectInfo setupProject(const QByteArray &projectFile, const QByteArray &mainFile,
                                ProjectExplorerHelper &pHelper)
{
    CppModelManagerHelper cppHelper;
    Project *project = pHelper.openProject(projectFilePath(_(projectFile)));
    if (!project)
        return ProjectInfo();

    // Wait only for a single file: we don't really care if the file is refreshed or not, but at
    // this point we know that the C++ model manager got notified of all project parts and we can
    // retrieve them for inspection.
    cppHelper.waitForSourceFilesRefreshed(projectFilePath(_(mainFile)));

    CppModelManager *mm = cppHelper.cppModelManager();
    return mm->projectInfo(project);
}

void GenericProjectPlugin::test_simple()
{
    ProjectExplorerHelper pHelper;

    const QByteArray mainFile("testdata_simpleproject/main.cpp");
    ProjectInfo pInfo(
                setupProject("testdata_simpleproject/simpleproject.creator", mainFile, pHelper));
    QVERIFY(pInfo);
    QCOMPARE(pInfo.projectParts().size(), 1);

    ProjectPart::Ptr pPart = pInfo.projectParts().first();
    QVERIFY(pPart);
    QCOMPARE(pPart->files.size(), 1);
    QCOMPARE(pPart->files.first().path, projectFilePath(_(mainFile)));
    QCOMPARE(pPart->files.first().kind, ProjectFile::CXXSource);
}

static QStringList simplify(const QList<CppTools::ProjectFile> &files, const QString &prefix)
{
    QStringList result;

    foreach (const CppTools::ProjectFile &file, files) {
        if (file.path.startsWith(prefix))
            result.append(file.path.mid(prefix.size()));
        else
            result.append(file.path);
    }

    return result;
}

void GenericProjectPlugin::test_mixed1()
{
    ProjectExplorerHelper pHelper;
    ProjectInfo pInfo(
                setupProject("testdata_mixedproject1/mixedproject1.creator",
                             "testdata_mixedproject1/main.cpp",
                             pHelper));
    QVERIFY(pInfo);
    QCOMPARE(pInfo.projectParts().size(), 3);

    QList<ProjectPart::Ptr> parts = pInfo.projectParts();
    std::sort(parts.begin(), parts.end(), [](const ProjectPart::Ptr &p1,
                                             const ProjectPart::Ptr &p2) {
        return p1->displayName < p2->displayName;
    });

    QStringList part0files = simplify(parts[0]->files,projectFilePath(_("testdata_mixedproject1/")));
    QStringList part1files = simplify(parts[1]->files,projectFilePath(_("testdata_mixedproject1/")));
    QStringList part2files = simplify(parts[2]->files,projectFilePath(_("testdata_mixedproject1/")));

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
    ProjectExplorerHelper pHelper;
    ProjectInfo pInfo(
                setupProject("testdata_mixedproject2/mixedproject2.creator",
                             "testdata_mixedproject2/main.cpp",
                             pHelper));
    QVERIFY(pInfo);
    QCOMPARE(pInfo.projectParts().size(), 2);

    QList<ProjectPart::Ptr> parts = pInfo.projectParts();
    std::sort(parts.begin(), parts.end(), [](const ProjectPart::Ptr &p1,
                                             const ProjectPart::Ptr &p2) {
        return p1->displayName < p2->displayName;
    });

    QStringList part0files = simplify(parts[0]->files,projectFilePath(_("testdata_mixedproject2/")));
    QStringList part1files = simplify(parts[1]->files,projectFilePath(_("testdata_mixedproject2/")));

    QCOMPARE(parts[0]->displayName, _("mixedproject2 (C++11)"));
    QCOMPARE(parts[0]->files.size(), 2);
    QVERIFY(part0files.contains(_("main.cpp")));
    QVERIFY(part0files.contains(_("header.hpp")));

    QCOMPARE(parts[1]->displayName, _("mixedproject2 (C11)"));
    QCOMPARE(parts[1]->files.size(), 1);
    QVERIFY(part1files.contains(_("impl.c")));
}
