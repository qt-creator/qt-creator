/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "abstractprocessstep.h"
#include "projectexplorer_global.h"

#include <utils/fileutils.h>

QT_FORWARD_DECLARE_CLASS(QListWidgetItem);

namespace Utils { class Environment; }

namespace ProjectExplorer {

namespace Internal {
namespace Ui { class MakeStep; }
} // namespace Internal

class PROJECTEXPLORER_EXPORT MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    enum MakeCommandType {
        Display,
        Execution
    };
    explicit MakeStep(ProjectExplorer::BuildStepList *parent, Core::Id id);

    void setBuildTarget(const QString &buildTarget);
    void setAvailableBuildTargets(const QStringList &buildTargets);

    bool init() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool buildsTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    QStringList availableTargets() const;
    QString userArguments() const;
    void setUserArguments(const QString &args);
    Utils::FilePath makeCommand() const;
    void setMakeCommand(const Utils::FilePath &command);
    Utils::FilePath makeExecutable() const;
    Utils::CommandLine effectiveMakeCommand(MakeCommandType type) const;

    void setClean(bool clean);
    bool isClean() const;

    static QString defaultDisplayName();

    Utils::FilePath defaultMakeCommand() const;
    static QString msgNoMakeCommand();
    static Task makeCommandMissingTask();

    virtual bool isJobCountSupported() const;
    int jobCount() const;
    void setJobCount(int count);
    bool jobCountOverridesMakeflags() const;
    void setJobCountOverrideMakeflags(bool override);
    bool makeflagsContainsJobCount() const;
    bool userArgsContainsJobCount() const;
    bool makeflagsJobCountMismatch() const;

    bool disablingForSubdirsSupported() const { return m_disablingForSubDirsSupported; }
    bool enabledForSubDirs() const { return m_enabledForSubDirs; }
    void setEnabledForSubDirs(bool enabled) { m_enabledForSubDirs = enabled; }

    Utils::Environment environment(BuildConfiguration *bc) const;

protected:
    bool fromMap(const QVariantMap &map) override;
    void supportDisablingForSubdirs() { m_disablingForSubDirsSupported = true; }
    virtual QStringList displayArguments() const;

private:
    QVariantMap toMap() const override;
    static int defaultJobCount();
    QStringList jobArguments() const;

    QStringList m_buildTargets;
    QStringList m_availableTargets;
    QString m_userArguments;
    Utils::FilePath m_makeCommand;
    int m_userJobCount = 4;
    bool m_overrideMakeflags = false;
    bool m_clean = false;
    bool m_disablingForSubDirsSupported = false;
    bool m_enabledForSubDirs = true;
};

class PROJECTEXPLORER_EXPORT MakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit MakeStepConfigWidget(MakeStep *makeStep);
    ~MakeStepConfigWidget() override;

private:
    void itemChanged(QListWidgetItem *item);
    void makeLineEditTextEdited();
    void makeArgumentsLineEditTextEdited();
    void updateDetails();
    void setUserJobCountVisible(bool visible);
    void setUserJobCountEnabled(bool enabled);

    Internal::Ui::MakeStep *m_ui;
    MakeStep *m_makeStep;
};

} // namespace GenericProjectManager
