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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "modelmanagertesthelper.h"

#include <QtTest>

#include <cassert>

using namespace CppTools::Internal;

TestProject::TestProject(const QString &name, QObject *parent)
    : m_name (name)
{
    setParent(parent);
    setId(Core::Id::fromString(name));
}

TestProject::~TestProject()
{
}

ModelManagerTestHelper::ModelManagerTestHelper(QObject *parent) :
    QObject(parent)

{
    CppModelManager *mm = CppModelManager::instance();
    assert(mm);

    connect(this, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)), mm, SLOT(onAboutToRemoveProject(ProjectExplorer::Project*)));
    connect(this, SIGNAL(projectAdded(ProjectExplorer::Project*)), mm, SLOT(onProjectAdded(ProjectExplorer::Project*)));
    connect(mm, SIGNAL(sourceFilesRefreshed(QStringList)), this, SLOT(sourceFilesRefreshed(QStringList)));
    connect(mm, SIGNAL(gcFinished()), this, SLOT(gcFinished()));

    cleanup();
    verifyClean();
}

ModelManagerTestHelper::~ModelManagerTestHelper()
{
    cleanup();
    verifyClean();
}

void ModelManagerTestHelper::cleanup()
{
    CppModelManager *mm = CppModelManager::instance();
    assert(mm);

    QList<ProjectInfo> pies = mm->projectInfos();
    foreach (const ProjectInfo &pie, pies)
        emit aboutToRemoveProject(pie.project().data());

    if (!pies.isEmpty())
        waitForFinishedGc();
}

void ModelManagerTestHelper::verifyClean()
{
    CppModelManager *mm = CppModelManager::instance();
    assert(mm);

    QVERIFY(mm->projectInfos().isEmpty());
    QVERIFY(mm->headerPaths().isEmpty());
    QVERIFY(mm->definedMacros().isEmpty());
    QVERIFY(mm->projectFiles().isEmpty());
    QVERIFY(mm->snapshot().isEmpty());
    QCOMPARE(mm->workingCopy().size(), 1);
    QVERIFY(mm->workingCopy().contains(mm->configurationFileName()));
}

ModelManagerTestHelper::Project *ModelManagerTestHelper::createProject(const QString &name)
{
    TestProject *tp = new TestProject(name, this);
    emit projectAdded(tp);
    return tp;
}

void ModelManagerTestHelper::resetRefreshedSourceFiles()
{
    m_lastRefreshedSourceFiles.clear();
    m_refreshHappened = false;
}

QStringList ModelManagerTestHelper::waitForRefreshedSourceFiles()
{
    while (!m_refreshHappened)
        QCoreApplication::processEvents();

    return m_lastRefreshedSourceFiles;
}

void ModelManagerTestHelper::waitForFinishedGc()
{
    m_gcFinished = false;

    while (!m_gcFinished)
        QCoreApplication::processEvents();
}

void ModelManagerTestHelper::sourceFilesRefreshed(const QStringList &files)
{
    m_lastRefreshedSourceFiles = files;
    m_refreshHappened = true;
}

void ModelManagerTestHelper::gcFinished()
{
    m_gcFinished = true;
}
