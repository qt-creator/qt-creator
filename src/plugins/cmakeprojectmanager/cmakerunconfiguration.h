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

#include <projectexplorer/runnables.h>
#include <utils/environment.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class CMakeRunConfigurationWidget;
    friend class ProjectExplorer::IRunConfigurationFactory;

public:
    explicit CMakeRunConfiguration(ProjectExplorer::Target *target);

    ProjectExplorer::Runnable runnable() const override;
    QWidget *createConfigurationWidget() override;

    void setExecutable(const QString &executable);
    void setBaseWorkingDirectory(const Utils::FileName &workingDirectory);

    QString title() const;

    QVariantMap toMap() const override;

    QString disabledReason() const override;

    QString buildSystemTarget() const final { return m_buildSystemTarget; }

private:
    void initialize(Core::Id id, const QString &target,
               const Utils::FileName &workingDirectory, const QString &title);
    void copyFrom(const CMakeRunConfiguration *source);

    bool fromMap(const QVariantMap &map) override;
    QString defaultDisplayName() const;

    void updateEnabledState() final;

    QString baseWorkingDirectory() const;

    QString m_buildSystemTarget;
    QString m_executable;
    QString m_title;
};

class CMakeRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent = 0);
};

class CMakeRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CMakeRunConfigurationFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *product) override;

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;

    static Core::Id idFromBuildTarget(const QString &target);
    static QString buildTargetFromId(Core::Id id);

private:
    bool canHandle(ProjectExplorer::Target *parent) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

} // namespace Internal
} // namespace CMakeProjectManager
