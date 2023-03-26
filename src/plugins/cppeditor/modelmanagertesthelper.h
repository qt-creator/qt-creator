// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppmodelmanager.h"

#include <projectexplorer/project.h>

#include <QObject>

namespace CppEditor::Tests {

class CPPEDITOR_EXPORT TestProject: public ProjectExplorer::Project
{
    Q_OBJECT

public:
    TestProject(const QString &name, QObject *parent, const Utils::FilePath &filePath = {});

    bool needsConfiguration() const final { return false; }

private:
    QString m_name;
};

class CPPEDITOR_EXPORT ModelManagerTestHelper: public QObject
{
    Q_OBJECT

public:
    using Project = ProjectExplorer::Project;

    explicit ModelManagerTestHelper(QObject *parent = nullptr,
                                    bool testOnlyForCleanedProjects = true);
    ~ModelManagerTestHelper() override;

    void cleanup();

    Project *createProject(const QString &name, const Utils::FilePath &filePath = {});

    QSet<Utils::FilePath> updateProjectInfo(const ProjectInfo::ConstPtr &projectInfo);

    void resetRefreshedSourceFiles();
    QSet<Utils::FilePath> waitForRefreshedSourceFiles();
    void waitForFinishedGc();

signals:
    void aboutToRemoveProject(Project *project);
    void projectAdded(Project*);

public slots:
    void sourceFilesRefreshed(const QSet<QString> &files);
    void gcFinished();

private:
    bool m_gcFinished;
    bool m_refreshHappened;
    bool m_testOnlyForCleanedProjects;
    QSet<QString> m_lastRefreshedSourceFiles;
    QList<Project *> m_projects;
};

} // namespace CppEditor::Tests
