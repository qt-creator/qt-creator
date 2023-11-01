// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelmanagertesthelper.h"

#include "cpptoolstestcase.h"
#include "projectinfo.h"

#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>

#include <QtTest>

#include <cassert>

using namespace Utils;

namespace CppEditor::Tests {

TestProject::TestProject(const QString &name, QObject *parent, const FilePath &filePath) :
    ProjectExplorer::Project("x-binary/foo", filePath),
    m_name(name)
{
    setParent(parent);
    setId(Id::fromString(name));
    setDisplayName(name);
    qRegisterMetaType<QSet<QString> >();
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
    QVERIFY(Internal::Tests::VerifyCleanCppModelManager::isClean(m_testOnlyForCleanedProjects));
}

ModelManagerTestHelper::~ModelManagerTestHelper()
{
    cleanup();
    QVERIFY(Internal::Tests::VerifyCleanCppModelManager::isClean(m_testOnlyForCleanedProjects));
}

void ModelManagerTestHelper::cleanup()
{
    CppModelManager *mm = CppModelManager::instance();
    QList<ProjectInfo::ConstPtr> pies = mm->projectInfos();
    for (Project * const p : std::as_const(m_projects)) {
        ProjectExplorer::ProjectManager::removeProject(p);
        emit aboutToRemoveProject(p);
    }

    if (!pies.isEmpty())
        waitForFinishedGc();
}

ModelManagerTestHelper::Project *ModelManagerTestHelper::createProject(
        const QString &name, const FilePath &filePath)
{
    auto tp = new TestProject(name, this, filePath);
    m_projects.push_back(tp);
    ProjectExplorer::ProjectManager::addProject(tp);
    emit projectAdded(tp);
    return tp;
}

QSet<FilePath> ModelManagerTestHelper::updateProjectInfo(
        const ProjectInfo::ConstPtr &projectInfo)
{
    resetRefreshedSourceFiles();
    CppModelManager::updateProjectInfo(projectInfo).waitForFinished();
    QCoreApplication::processEvents();
    return waitForRefreshedSourceFiles();
}

void ModelManagerTestHelper::resetRefreshedSourceFiles()
{
    m_lastRefreshedSourceFiles.clear();
    m_refreshHappened = false;
}

QSet<FilePath> ModelManagerTestHelper::waitForRefreshedSourceFiles()
{
    while (!m_refreshHappened)
        QCoreApplication::processEvents();

    return Utils::transform(m_lastRefreshedSourceFiles, &FilePath::fromString);
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

} // CppEditor::Tests
