/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#ifndef TESTCONFIGURATION_H
#define TESTCONFIGURATION_H

#include <utils/environment.h>

#include <QObject>
#include <QStringList>

namespace ProjectExplorer {
class Project;
}

namespace Autotest {
namespace Internal {

class TestConfiguration : public QObject

{
    Q_OBJECT
public:
    explicit TestConfiguration(const QString &testClass, const QStringList &testCases,
                               int testCaseCount = 0, QObject *parent = 0);
    ~TestConfiguration();

    void setTestCases(const QStringList &testCases);
    void setTestCaseCount(int count);
    void setTargetFile(const QString &targetFile);
    void setTargetName(const QString &targetName);
    void setProFile(const QString &proFile);
    void setWorkingDirectory(const QString &workingDirectory);
    void setDisplayName(const QString &displayName);
    void setEnvironment(const Utils::Environment &env);
    void setProject(ProjectExplorer::Project *project);
    void setUnnamedOnly(bool unnamedOnly);

    QString testClass() const { return m_testClass; }
    QStringList testCases() const { return m_testCases; }
    int testCaseCount() const { return m_testCaseCount; }
    QString proFile() const { return m_proFile; }
    QString targetFile() const { return m_targetFile; }
    QString targetName() const { return m_targetName; }
    QString workingDirectory() const { return m_workingDir; }
    QString displayName() const { return m_displayName; }
    Utils::Environment environment() const { return m_environment; }
    ProjectExplorer::Project *project() const { return m_project; }
    bool unnamedOnly() const { return m_unnamedOnly; }


signals:

public slots:

private:
    QString m_testClass;
    QStringList m_testCases;
    int m_testCaseCount;
    bool m_unnamedOnly;
    QString m_proFile;
    QString m_targetFile;
    QString m_targetName;
    QString m_workingDir;
    QString m_displayName;
    Utils::Environment m_environment;
    ProjectExplorer::Project *m_project;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTCONFIGURATION_H
