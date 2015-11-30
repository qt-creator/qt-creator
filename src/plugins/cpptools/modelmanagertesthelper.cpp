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

#include "modelmanagertesthelper.h"

#include "cpptoolstestcase.h"
#include "cppworkingcopy.h"

#include <QtTest>

#include <cassert>

Q_DECLARE_METATYPE(QSet<QString>)

using namespace CppTools::Internal;
using namespace CppTools::Tests;

TestProject::TestProject(const QString &name, QObject *parent)
    : m_name (name)
{
    setParent(parent);
    setId(Core::Id::fromString(name));
    qRegisterMetaType<QSet<QString> >();
}

TestProject::~TestProject()
{
}

ModelManagerTestHelper::ModelManagerTestHelper(QObject *parent,
                                               bool testOnlyForCleanedProjects)
    : QObject(parent)
    , m_testOnlyForCleanedProjects(testOnlyForCleanedProjects)

{
    CppModelManager *mm = CppModelManager::instance();
    connect(this, &ModelManagerTestHelper::aboutToRemoveProject,
            mm, &CppModelManager::onAboutToRemoveProject);
    connect(this, &ModelManagerTestHelper::projectAdded,
            mm, &CppModelManager::onProjectAdded);
    connect(mm, &CppModelManager::sourceFilesRefreshed,
            this, &ModelManagerTestHelper::sourceFilesRefreshed);
    connect(mm, &CppModelManager::gcFinished,
            this, &ModelManagerTestHelper::gcFinished);

    cleanup();
    QVERIFY(Tests::VerifyCleanCppModelManager::isClean(m_testOnlyForCleanedProjects));
}

ModelManagerTestHelper::~ModelManagerTestHelper()
{
    cleanup();
    QVERIFY(Tests::VerifyCleanCppModelManager::isClean(m_testOnlyForCleanedProjects));
}

void ModelManagerTestHelper::cleanup()
{
    CppModelManager *mm = CppModelManager::instance();
    QList<ProjectInfo> pies = mm->projectInfos();
    foreach (const ProjectInfo &pie, pies)
        emit aboutToRemoveProject(pie.project().data());

    if (!pies.isEmpty())
        waitForFinishedGc();
}

ModelManagerTestHelper::Project *ModelManagerTestHelper::createProject(const QString &name)
{
    TestProject *tp = new TestProject(name, this);
    emit projectAdded(tp);
    return tp;
}

QSet<QString> ModelManagerTestHelper::updateProjectInfo(const CppTools::ProjectInfo &projectInfo)
{
    resetRefreshedSourceFiles();
    CppModelManager::instance()->updateProjectInfo(projectInfo).waitForFinished();
    QCoreApplication::processEvents();
    return waitForRefreshedSourceFiles();
}

void ModelManagerTestHelper::resetRefreshedSourceFiles()
{
    m_lastRefreshedSourceFiles.clear();
    m_refreshHappened = false;
}

QSet<QString> ModelManagerTestHelper::waitForRefreshedSourceFiles()
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

void ModelManagerTestHelper::sourceFilesRefreshed(const QSet<QString> &files)
{
    m_lastRefreshedSourceFiles = files;
    m_refreshHappened = true;
}

void ModelManagerTestHelper::gcFinished()
{
    m_gcFinished = true;
}
