/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QT4RUNCONFIGURATION_H
#define QT4RUNCONFIGURATION_H

#include "qmakerunconfigurationfactory.h"

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <utils/environment.h>

#include <QStringList>
#include <QMetaType>
#include <QLabel>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QRadioButton;
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
class DetailsWidget;
}

namespace ProjectExplorer {
    class EnvironmentWidget;
}

namespace Qt4ProjectManager {

class Qt4Project;
class Qt4ProFileNode;
class Qt4PriFileNode;

namespace Internal {
class Qt4RunConfigurationFactory;

class Qt4RunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    // to change the display name and arguments and set the userenvironmentchanges
    friend class Qt4RunConfigurationWidget;
    friend class Qt4RunConfigurationFactory;

public:
    Qt4RunConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    virtual ~Qt4RunConfiguration();

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    virtual QWidget *createConfigurationWidget();

    virtual QString executable() const;
    virtual RunMode runMode() const;
    bool forcedGuiMode() const;
    virtual QString workingDirectory() const;
    virtual QString commandLineArguments() const;
    virtual Utils::Environment environment() const;
    QString dumperLibrary() const;
    QStringList dumperLibraryLocations() const;

    bool isUsingDyldImageSuffix() const;
    void setUsingDyldImageSuffix(bool state);

    QString proFilePath() const;

    QVariantMap toMap() const;

    Utils::OutputFormatter *createOutputFormatter() const;

    void setRunMode(RunMode runMode);
signals:
    void commandLineArgumentsChanged(const QString&);
    void baseWorkingDirectoryChanged(const QString&);
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &diff);

    // Note: These signals might not get emitted for every change!
    void effectiveTargetInformationChanged();

private slots:
    void kitChanged();
    void proFileUpdated(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress);

protected:
    Qt4RunConfiguration(ProjectExplorer::Target *parent, Qt4RunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setCommandLineArguments(const QString &argumentsString);
    QString rawCommandLineArguments() const;
    enum BaseEnvironmentBase { CleanEnvironmentBase = 0,
                               SystemEnvironmentBase = 1,
                               BuildEnvironmentBase  = 2 };
    QString defaultDisplayName();
    void setBaseEnvironmentBase(BaseEnvironmentBase env);
    BaseEnvironmentBase baseEnvironmentBase() const;

    void ctor();

    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;

    void updateTarget();
    QString m_commandLineArguments;
    QString m_proFilePath; // Full path to the Application Pro File

    // Cached startup sub project information
    ProjectExplorer::LocalApplicationRunConfiguration::RunMode m_runMode;
    bool m_forcedGuiMode;
    bool m_userSetName;
    bool m_isUsingDyldImageSuffix;
    QString m_userWorkingDirectory;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    BaseEnvironmentBase m_baseEnvironmentBase;
    bool m_parseSuccess;
    bool m_parseInProgress;
};

class Qt4RunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    Qt4RunConfigurationWidget(Qt4RunConfiguration *qt4runconfigration, QWidget *parent);
    ~Qt4RunConfigurationWidget();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void runConfigurationEnabledChange();
    void workDirectoryEdited();
    void workingDirectoryReseted();
    void argumentsEdited(const QString &arguments);
    void userChangesEdited();

    void workingDirectoryChanged(const QString &workingDirectory);
    void commandLineArgumentsChanged(const QString &args);
    void runModeChanged(ProjectExplorer::LocalApplicationRunConfiguration::RunMode runMode);
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges);
    void baseEnvironmentChanged();

    void effectiveTargetInformationChanged();
    void termToggled(bool);
    void usingDyldImageSuffixToggled(bool);
    void usingDyldImageSuffixChanged(bool);
    void baseEnvironmentSelected(int index);

private:
    Qt4RunConfiguration *m_qt4RunConfiguration;
    bool m_ignoreChange;
    QLabel *m_disabledIcon;
    QLabel *m_disabledReason;
    QLineEdit *m_executableLineEdit;
    Utils::PathChooser *m_workingDirectoryEdit;
    QLineEdit *m_argumentsLineEdit;
    QCheckBox *m_useTerminalCheck;
    QCheckBox *m_usingDyldImageSuffix;
    QLineEdit *m_qmlDebugPort;

    QComboBox *m_baseEnvironmentComboBox;
    Utils::DetailsWidget *m_detailsContainer;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    bool m_isShown;
};

class Qt4RunConfigurationFactory : public QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4RunConfigurationFactory(QObject *parent = 0);
    ~Qt4RunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canHandle(ProjectExplorer::Target *t) const;
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        ProjectExplorer::Node *n);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4RUNCONFIGURATION_H
