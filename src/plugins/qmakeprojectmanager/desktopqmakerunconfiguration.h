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

#ifndef DESKTOPQMAKERUNCONFIGURATION_H
#define DESKTOPQMAKERUNCONFIGURATION_H

#include <qmakeprojectmanager/qmakerunconfigurationfactory.h>

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <utils/fileutils.h>

#include <QStringList>
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

namespace QmakeProjectManager {

class QmakeProFileNode;

namespace Internal {
class DesktopQmakeRunConfigurationFactory;

class DesktopQmakeRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    // to change the display name and arguments and set the userenvironmentchanges
    friend class DesktopQmakeRunConfigurationWidget;
    friend class DesktopQmakeRunConfigurationFactory;

public:
    DesktopQmakeRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    bool isEnabled() const override;
    QString disabledReason() const override;
    QWidget *createConfigurationWidget() override;

    QString executable() const override;
    ProjectExplorer::ApplicationLauncher::Mode runMode() const override;
    QString workingDirectory() const override;
    QString commandLineArguments() const override;

    bool isUsingDyldImageSuffix() const;
    void setUsingDyldImageSuffix(bool state);

    bool isUsingLibrarySearchPath() const;
    void setUsingLibrarySearchPath(bool state);

    Utils::FileName proFilePath() const;

    QVariantMap toMap() const override;

    Utils::OutputFormatter *createOutputFormatter() const override;

    void setRunMode(ProjectExplorer::ApplicationLauncher::Mode runMode);

    void addToBaseEnvironment(Utils::Environment &env) const override;
signals:
    void commandLineArgumentsChanged(const QString&);
    void baseWorkingDirectoryChanged(const QString&);
    void runModeChanged(ProjectExplorer::ApplicationLauncher::Mode runMode);
    void usingDyldImageSuffixChanged(bool);
    void usingLibrarySearchPathChanged(bool);

    // Note: These signals might not get emitted for every change!
    void effectiveTargetInformationChanged();

private slots:
    void proFileUpdated(QmakeProjectManager::QmakeProFileNode *pro, bool success, bool parseInProgress);
    void proFileEvaluated();

protected:
    DesktopQmakeRunConfiguration(ProjectExplorer::Target *parent, DesktopQmakeRunConfiguration *source);
    bool fromMap(const QVariantMap &map) override;

private:
    QPair<QString, QString> extractWorkingDirAndExecutable(const QmakeProFileNode *node) const;
    void setBaseWorkingDirectory(const QString &workingDirectory);
    QString baseWorkingDirectory() const;
    void setCommandLineArguments(const QString &argumentsString);
    QString rawCommandLineArguments() const;
    QString defaultDisplayName();

    void ctor();

    void updateTarget();
    QString m_commandLineArguments;
    Utils::FileName m_proFilePath; // Full path to the Application Pro File

    // Cached startup sub project information
    ProjectExplorer::ApplicationLauncher::Mode m_runMode = ProjectExplorer::ApplicationLauncher::Gui;
    bool m_isUsingDyldImageSuffix = false;
    bool m_isUsingLibrarySearchPath = true;
    QString m_userWorkingDirectory;
    bool m_parseSuccess = false;
    bool m_parseInProgress = false;
};

class DesktopQmakeRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    DesktopQmakeRunConfigurationWidget(DesktopQmakeRunConfiguration *qmakeRunConfiguration, QWidget *parent);
    ~DesktopQmakeRunConfigurationWidget();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void runConfigurationEnabledChange();
    void workDirectoryEdited();
    void workingDirectoryReseted();
    void argumentsEdited(const QString &arguments);
    void environmentWasChanged();

    void workingDirectoryChanged(const QString &workingDirectory);
    void commandLineArgumentsChanged(const QString &args);
    void runModeChanged(ProjectExplorer::ApplicationLauncher::Mode runMode);

    void effectiveTargetInformationChanged();
    void termToggled(bool);
    void qvfbToggled(bool);
    void usingDyldImageSuffixToggled(bool);
    void usingDyldImageSuffixChanged(bool);
    void usingLibrarySearchPathToggled(bool state);
    void usingLibrarySearchPathChanged(bool state);

private:
    DesktopQmakeRunConfiguration *m_qmakeRunConfiguration = nullptr;
    bool m_ignoreChange = false;
    QLabel *m_disabledIcon = nullptr;
    QLabel *m_disabledReason = nullptr;
    QLabel *m_executableLineLabel = nullptr;
    Utils::PathChooser *m_workingDirectoryEdit = nullptr;
    QLineEdit *m_argumentsLineEdit = nullptr;
    QCheckBox *m_useTerminalCheck = nullptr;
    QCheckBox *m_useQvfbCheck = nullptr;
    QCheckBox *m_usingDyldImageSuffix = nullptr;
    QCheckBox *m_usingLibrarySearchPath = nullptr;
    QLineEdit *m_qmlDebugPort = nullptr;
    Utils::DetailsWidget *m_detailsContainer;
    bool m_isShown = false;
};

class DesktopQmakeRunConfigurationFactory : public QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit DesktopQmakeRunConfigurationFactory(QObject *parent = 0);
    ~DesktopQmakeRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const;
    QString displayNameForId(Core::Id id) const;

    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        const ProjectExplorer::Node *n);

private:
    bool canHandle(ProjectExplorer::Target *t) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id);
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map);
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // DESKTOPQMAKERUNCONFIGURATION_H
