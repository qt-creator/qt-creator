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
    static ProjectExplorerPlugin *getInstance()
    { return ProjectExplorerPlugin::instance(); }

    ProjectExplorerHelper()
    {
        QVERIFY(!SessionManager::hasProjects());
    }

    ~ProjectExplorerHelper()
    {
        foreach (Project *project, m_openProjects)
            getInstance()->unloadProject(project);
    }

    Project *openProject(const QString &projectFile)
    {
        QString error;
        Project *project = getInstance()->openProject(projectFile, &error);
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

void GenericProjectPlugin::test_simple()
{
    CppModelManagerHelper cppHelper;

    QString projectFile = _("testdata_simpleproject/simpleproject.creator");
    ProjectExplorerHelper pHelper;
    Project *project = pHelper.openProject(projectFilePath(projectFile));
    QVERIFY(project);

    QString mainFile = projectFilePath(_("testdata_simpleproject/main.cpp"));
    cppHelper.waitForSourceFilesRefreshed(mainFile);

    CppModelManagerInterface *mm = cppHelper.cppModelManager();
    ProjectInfo pInfo = mm->projectInfo(project);

    QCOMPARE(pInfo.projectParts().size(), 1);

    ProjectPart::Ptr pPart = pInfo.projectParts().first();
    QVERIFY(pPart);
    QCOMPARE(pPart->files.size(), 1);
    QCOMPARE(pPart->files.first().path, mainFile);
    QCOMPARE(pPart->files.first().kind, ProjectFile::CXXSource);
}
