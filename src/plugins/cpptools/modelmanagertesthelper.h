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

#pragma once

#include "cpptools_global.h"
#include "cppmodelmanager.h"

#include <projectexplorer/project.h>

#include <QObject>

namespace CppTools {
namespace Tests {

class CPPTOOLS_EXPORT TestProject: public ProjectExplorer::Project
{
    Q_OBJECT

public:
    TestProject(const QString &name, QObject *parent);

private:
    QString m_name;
};

class CPPTOOLS_EXPORT ModelManagerTestHelper: public QObject
{
    Q_OBJECT

public:
    typedef ProjectExplorer::Project Project;

    explicit ModelManagerTestHelper(QObject *parent = 0,
                                    bool testOnlyForCleanedProjects = true);
    ~ModelManagerTestHelper() override;

    void cleanup();

    Project *createProject(const QString &name);

    QSet<QString> updateProjectInfo(const ProjectInfo &projectInfo);

    void resetRefreshedSourceFiles();
    QSet<QString> waitForRefreshedSourceFiles();
    void waitForFinishedGc();

signals:
    void aboutToRemoveProject(ProjectExplorer::Project *project);
    void projectAdded(ProjectExplorer::Project*);

public slots:
    void sourceFilesRefreshed(const QSet<QString> &files);
    void gcFinished();

private:
    bool m_gcFinished;
    bool m_refreshHappened;
    bool m_testOnlyForCleanedProjects;
    QSet<QString> m_lastRefreshedSourceFiles;
};

} // namespace Tests
} // namespace CppTools
