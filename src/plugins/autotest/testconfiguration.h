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

#ifndef TESTCONFIGURATION_H
#define TESTCONFIGURATION_H

#include "autotestconstants.h"

#include <projectexplorer/project.h>
#include <utils/environment.h>

#include <QObject>
#include <QPointer>
#include <QStringList>

namespace Autotest {
namespace Internal {

class TestConfiguration : public QObject

{
    Q_OBJECT
public:
    explicit TestConfiguration(const QString &testClass, const QStringList &testCases,
                               int testCaseCount = 0, QObject *parent = 0);
    ~TestConfiguration();

    void completeTestInformation();

    void setTestCases(const QStringList &testCases);
    void setTestCaseCount(int count);
    void setTargetFile(const QString &targetFile);
    void setTargetName(const QString &targetName);
    void setProFile(const QString &proFile);
    void setWorkingDirectory(const QString &workingDirectory);
    void setBuildDirectory(const QString &buildDirectory);
    void setDisplayName(const QString &displayName);
    void setEnvironment(const Utils::Environment &env);
    void setProject(ProjectExplorer::Project *project);
    void setUnnamedOnly(bool unnamedOnly);
    void setGuessedConfiguration(bool guessed);
    void setTestType(TestType type);

    QString testClass() const { return m_testClass; }
    QStringList testCases() const { return m_testCases; }
    int testCaseCount() const { return m_testCaseCount; }
    QString proFile() const { return m_proFile; }
    QString targetFile() const { return m_targetFile; }
    QString targetName() const { return m_targetName; }
    QString workingDirectory() const { return m_workingDir; }
    QString buildDirectory() const { return m_buildDir; }
    QString displayName() const { return m_displayName; }
    Utils::Environment environment() const { return m_environment; }
    ProjectExplorer::Project *project() const { return m_project.data(); }
    bool unnamedOnly() const { return m_unnamedOnly; }
    bool guessedConfiguration() const { return m_guessedConfiguration; }
    TestType testType() const { return m_type; }

private:
    QString m_testClass;
    QStringList m_testCases;
    int m_testCaseCount;
    QString m_mainFilePath;
    bool m_unnamedOnly;
    QString m_proFile;
    QString m_targetFile;
    QString m_targetName;
    QString m_workingDir;
    QString m_buildDir;
    QString m_displayName;
    Utils::Environment m_environment;
    QPointer<ProjectExplorer::Project> m_project;
    bool m_guessedConfiguration;
    TestType m_type;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTCONFIGURATION_H
