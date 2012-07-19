/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CMAKEOPENPROJECTWIZARD_H
#define CMAKEOPENPROJECTWIZARD_H

#include <utils/environment.h>
#include <utils/wizard.h>
#include <utils/qtcprocess.h>

#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>

namespace Utils {
    class PathChooser;
}

namespace ProjectExplorer {
class ToolChain;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeManager;

class CMakeOpenProjectWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    enum PageId {
        InSourcePageId,
        ShadowBuildPageId,
        CMakeRunPageId
    };

    enum Mode {
        Nothing,
        NeedToCreate,
        NeedToUpdate,
        WantToUpdate
    };

    // used at importing a project without a .user file
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, const Utils::Environment &env);
    /// used to update if we have already a .user file
    /// recreates or updates the cbp file
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, const QString &buildDirectory, Mode mode, const Utils::Environment &env);
    /// used to change the build directory of one buildconfiguration
    /// shows a page for selecting a directory
    /// then the run cmake page
    CMakeOpenProjectWizard(CMakeManager *cmakeManager, const QString &sourceDirectory, const QString &oldBuildDirectory, const Utils::Environment &env);

    virtual int nextId() const;
    QString buildDirectory() const;
    QString sourceDirectory() const;
    void setBuildDirectory(const QString &directory);
    CMakeManager *cmakeManager() const;
    QString arguments() const;
    void setArguments(const QString &args);
    Utils::Environment environment() const;
    bool existsUpToDateXmlFile() const;

private:
    void init();
    bool hasInSourceBuild() const;
    CMakeManager *m_cmakeManager;
    QString m_buildDirectory;
    QString m_sourceDirectory;
    QString m_arguments;
    bool m_creatingCbpFiles;
    Utils::Environment m_environment;
};

class InSourceBuildPage : public QWizardPage
{
    Q_OBJECT
public:
    InSourceBuildPage(CMakeOpenProjectWizard *cmakeWizard);
private:
    CMakeOpenProjectWizard *m_cmakeWizard;
};

class ShadowBuildPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ShadowBuildPage(CMakeOpenProjectWizard *cmakeWizard, bool change = false);
private slots:
    void buildDirectoryChanged();
private:
    CMakeOpenProjectWizard *m_cmakeWizard;
    Utils::PathChooser *m_pc;
};

class CMakeRunPage : public QWizardPage
{
    Q_OBJECT
public:
    enum Mode { Initial, NeedToUpdate, Recreate, ChangeDirectory, WantToUpdate };
    explicit CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, Mode mode = Initial, const QString &buildDirectory = QString());

    virtual void initializePage();
    virtual void cleanupPage();
    virtual bool isComplete() const;
private slots:
    void runCMake();
    void cmakeFinished();
    void cmakeReadyReadStandardOutput();
    void cmakeReadyReadStandardError();
private:
    void initWidgets();
    CMakeOpenProjectWizard *m_cmakeWizard;
    QPlainTextEdit *m_output;
    QPushButton *m_runCMake;
    Utils::QtcProcess *m_cmakeProcess;
    QLineEdit *m_argumentsLineEdit;
    Utils::PathChooser *m_cmakeExecutable;
    QComboBox *m_generatorComboBox;
    QLabel *m_descriptionLabel;
    QLabel *m_exitCodeLabel;
    bool m_complete;
    Mode m_mode;
    QString m_buildDirectory;
};

}
}

#endif // CMAKEOPENPROJECTWIZARD_H
