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

#ifndef CMAKEOPENPROJECTWIZARD_H
#define CMAKEOPENPROJECTWIZARD_H

#include "cmakebuildconfiguration.h"
#include "cmakebuildinfo.h"

#include <utils/environment.h>
#include <utils/wizard.h>
#include <utils/qtcprocess.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>

#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class PathChooser;
}

namespace ProjectExplorer { class Kit; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeManager;

class CMakeOpenProjectWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    enum Mode {
        Nothing,
        NeedToCreate,
        NeedToUpdate,
        WantToUpdate,
        ChangeDirectory
    };

    /// used to update if we have already a .user file
    /// recreates or updates the cbp file
    /// Also used to change the build directory of one buildconfiguration or create a new buildconfiguration
    CMakeOpenProjectWizard(QWidget *parent, CMakeManager *cmakeManager, Mode mode,
                           const CMakeBuildInfo *info);

    QString buildDirectory() const;
    QString sourceDirectory() const;
    void setBuildDirectory(const QString &directory);
    CMakeManager *cmakeManager() const;
    QString arguments() const;
    void setArguments(const QString &args);
    Utils::Environment environment() const;
    ProjectExplorer::Kit *kit() const;
    void setKit(ProjectExplorer::Kit *kit);
    bool existsUpToDateXmlFile() const;
    bool compatibleKitExist() const;

private:
    bool hasInSourceBuild() const;
    CMakeManager *m_cmakeManager;
    QString m_buildDirectory;
    QString m_sourceDirectory;
    QString m_arguments;
    Utils::Environment m_environment;
    ProjectExplorer::Kit *m_kit;
};

class NoKitPage : public QWizardPage
{
    Q_OBJECT
public:
    NoKitPage(CMakeOpenProjectWizard *cmakeWizard);
    bool isComplete() const override;
    void initializePage() override;

private:
    void kitsChanged();
    void showOptions();

    QLabel *m_descriptionLabel;
    QPushButton *m_optionsButton;
    CMakeOpenProjectWizard *m_cmakeWizard;
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

private:
    void buildDirectoryChanged();

    CMakeOpenProjectWizard *m_cmakeWizard;
    Utils::PathChooser *m_pc;
};

class NoCMakePage : public QWizardPage
{
    Q_OBJECT
public:
    NoCMakePage(CMakeOpenProjectWizard *cmakeWizard);
    bool isComplete() const override;

private:
    void cmakeToolsChanged();
    void showOptions();

    QLabel *m_descriptionLabel;
    QPushButton *m_optionsButton;
};

class CMakeRunPage : public QWizardPage
{
    Q_OBJECT
public:
    enum Mode { NeedToUpdate, Recreate, ChangeDirectory, WantToUpdate };
    explicit CMakeRunPage(CMakeOpenProjectWizard *cmakeWizard, Mode mode,
                          const QString &buildDirectory,
                          const QString &initialArguments,
                          const QString &kitName,
                          const QString &buildConfigurationName);

    void initializePage() override;
    bool validatePage() override;
    void cleanupPage() override;
    bool isComplete() const override;

private:
    void runCMake();
    void cmakeFinished();
    void cmakeReadyReadStandardOutput();
    void cmakeReadyReadStandardError();

    QByteArray cachedGeneratorFromFile(const QString &cache);

    CMakeOpenProjectWizard *m_cmakeWizard;
    QLabel *m_descriptionLabel;
    Utils::FancyLineEdit *m_argumentsLineEdit;
    QComboBox *m_generatorComboBox;
    QLabel *m_generatorExtraText;
    QPushButton *m_runCMake;
    QPlainTextEdit *m_output;
    QLabel *m_exitCodeLabel;
    QCheckBox *m_continueCheckBox;
    Utils::QtcProcess *m_cmakeProcess = 0;
    bool m_haveCbpFile = false;
    Mode m_mode;
    QString m_buildDirectory;
    QString m_kitName;
    QString m_buildConfigurationName;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEOPENPROJECTWIZARD_H
