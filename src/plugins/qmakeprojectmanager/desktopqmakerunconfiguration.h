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

#include <qmakeprojectmanager/qmakerunconfigurationfactory.h>

#include <projectexplorer/runnables.h>

#include <utils/fileutils.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace QmakeProjectManager {

class QmakeProFile;
class QmakeProject;

namespace Internal {
class DesktopQmakeRunConfigurationFactory;

class DesktopQmakeRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    // to change the display name and arguments and set the userenvironmentchanges
    friend class DesktopQmakeRunConfigurationWidget;
    friend class ProjectExplorer::IRunConfigurationFactory;

public:
    explicit DesktopQmakeRunConfiguration(ProjectExplorer::Target *target);

    QWidget *createConfigurationWidget() override;

    ProjectExplorer::Runnable runnable() const override;
    QString executable() const;

    bool isUsingDyldImageSuffix() const;
    void setUsingDyldImageSuffix(bool state);

    bool isUsingLibrarySearchPath() const;
    void setUsingLibrarySearchPath(bool state);

    Utils::FileName proFilePath() const;

    QVariantMap toMap() const override;

    Utils::OutputFormatter *createOutputFormatter() const override;

    void addToBaseEnvironment(Utils::Environment &env) const;

    QString buildSystemTarget() const final;

signals:
    void baseWorkingDirectoryChanged(const QString&);
    void usingDyldImageSuffixChanged(bool);
    void usingLibrarySearchPathChanged(bool);

    // Note: These signals might not get emitted for every change!
    void effectiveTargetInformationChanged();

protected:
    void initialize(Core::Id id);
    void copyFrom(const DesktopQmakeRunConfiguration *source);

    bool fromMap(const QVariantMap &map) override;

private:
    void proFileEvaluated();
    void updateTargetInformation();

    QPair<QString, QString> extractWorkingDirAndExecutable(const QmakeProFile *proFile) const;
    QString baseWorkingDirectory() const;
    QString defaultDisplayName();
    bool isConsoleApplication() const;
    QmakeProject *qmakeProject() const;
    QmakeProFile *proFile() const;

    void ctor();

    void updateTarget();
    Utils::FileName m_proFilePath; // Full path to the Application Pro File

    // Cached startup sub project information
    bool m_isUsingDyldImageSuffix = false;
    bool m_isUsingLibrarySearchPath = true;
};

class DesktopQmakeRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopQmakeRunConfigurationWidget(DesktopQmakeRunConfiguration *qmakeRunConfiguration);

private:
    void effectiveTargetInformationChanged();
    void usingDyldImageSuffixToggled(bool);
    void usingDyldImageSuffixChanged(bool);
    void usingLibrarySearchPathToggled(bool state);
    void usingLibrarySearchPathChanged(bool state);

private:
    DesktopQmakeRunConfiguration *m_qmakeRunConfiguration = nullptr;
    bool m_ignoreChange = false;
    QLabel *m_executableLineLabel = nullptr;
    QCheckBox *m_useQvfbCheck = nullptr;
    QCheckBox *m_usingDyldImageSuffix = nullptr;
    QCheckBox *m_usingLibrarySearchPath = nullptr;
    QLineEdit *m_qmlDebugPort = nullptr;
};

class DesktopQmakeRunConfigurationFactory : public QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit DesktopQmakeRunConfigurationFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *source) override;

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;

    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        const ProjectExplorer::Node *n) override;

private:
    bool canHandle(ProjectExplorer::Target *t) const override;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

} // namespace Internal
} // namespace QmakeProjectManager
