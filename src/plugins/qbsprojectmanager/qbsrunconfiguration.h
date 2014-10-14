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

#ifndef QBSRUNCONFIGURATION_H
#define QBSRUNCONFIGURATION_H

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <QStringList>
#include <QLabel>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
class QComboBox;
QT_END_NAMESPACE

namespace qbs { class InstallOptions; }

namespace Utils {
class PathChooser;
class DetailsWidget;
}

namespace ProjectExplorer { class BuildStepList; }

namespace QbsProjectManager {

class QbsProject;

namespace Internal {

class QbsInstallStep;
class QbsRunConfigurationFactory;

class QbsRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT

    // to change the display name and arguments and set the userenvironmentchanges
    friend class QbsRunConfigurationWidget;
    friend class QbsRunConfigurationFactory;

public:
    QbsRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();

    QString executable() const;
    ProjectExplorer::ApplicationLauncher::Mode runMode() const;
    QString workingDirectory() const;
    QString commandLineArguments() const;

    QVariantMap toMap() const;

    Utils::OutputFormatter *createOutputFormatter() const;

    void setRunMode(ProjectExplorer::ApplicationLauncher::Mode runMode);

    void addToBaseEnvironment(Utils::Environment &env) const;

    QString uniqueProductName() const;
    bool isConsoleApplication() const;

signals:
    void commandLineArgumentsChanged(const QString&);
    void baseWorkingDirectoryChanged(const QString&);
    void runModeChanged(ProjectExplorer::ApplicationLauncher::Mode runMode);
    void targetInformationChanged();
    void usingDyldImageSuffixChanged(bool);

protected:
    QbsRunConfiguration(ProjectExplorer::Target *parent, QbsRunConfiguration *source);
    bool fromMap(const QVariantMap &map);

private slots:
    void installStepChanged();
    void installStepToBeRemoved(int pos);

private:
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setCommandLineArguments(const QString &argumentsString);
    QString rawCommandLineArguments() const;
    QString defaultDisplayName();
    qbs::InstallOptions installOptions() const;
    QString installRoot() const;

    void ctor();

    void updateTarget();

    QString m_uniqueProductName;
    QString m_commandLineArguments;

    // Cached startup sub project information
    ProjectExplorer::ApplicationLauncher::Mode m_runMode;
    QString m_userWorkingDirectory;

    QbsInstallStep *m_currentInstallStep; // We do not take ownership!
    ProjectExplorer::BuildStepList *m_currentBuildStepList; // We do not take ownership!
};

class QbsRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    QbsRunConfigurationWidget(QbsRunConfiguration *rc, QWidget *parent);

private slots:
    void runConfigurationEnabledChange();
    void workDirectoryEdited();
    void workingDirectoryWasReset();
    void argumentsEdited(const QString &arguments);
    void environmentWasChanged();

    void workingDirectoryChanged(const QString &workingDirectory);
    void commandLineArgumentsChanged(const QString &args);
    void runModeChanged(ProjectExplorer::ApplicationLauncher::Mode runMode);

    void termToggled(bool);

private slots:
    void targetInformationHasChanged();

private:
    QbsRunConfiguration *m_rc;
    bool m_ignoreChange;
    QLabel *m_disabledIcon;
    QLabel *m_disabledReason;
    QLineEdit *m_executableLineEdit;
    Utils::PathChooser *m_workingDirectoryEdit;
    QLineEdit *m_argumentsLineEdit;
    QCheckBox *m_useTerminalCheck;
    QCheckBox *m_usingDyldImageSuffix;
    QLineEdit *m_qmlDebugPort;
    Utils::DetailsWidget *m_detailsContainer;
    bool m_isShown;
};

class QbsRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit QbsRunConfigurationFactory(QObject *parent = 0);
    ~QbsRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const;
    QString displayNameForId(Core::Id id) const;

private:
    bool canHandle(ProjectExplorer::Target *t) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id);
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map);
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSRUNCONFIGURATION_H
