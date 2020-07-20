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

#include <utils/fileutils.h>

namespace Utils { class Environment; }

namespace ProjectExplorer {

namespace Internal {
class MakeStepConfigWidget;
class OverrideMakeflagsAspect;
} // Internal

class BaseBoolAspect;
class BaseIntegerAspect;
class BaseStringAspect;
class BaseStringListAspect;

class PROJECTEXPLORER_EXPORT MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    enum MakeCommandType {
        Display,
        Execution
    };
    explicit MakeStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);

    void setBuildTarget(const QString &buildTarget);
    void setAvailableBuildTargets(const QStringList &buildTargets);

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
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
    bool jobCountOverridesMakeflags() const;
    bool makeflagsContainsJobCount() const;
    bool userArgsContainsJobCount() const;
    bool makeflagsJobCountMismatch() const;

    bool disablingForSubdirsSupported() const { return m_disablingForSubDirsSupported; }
    bool enabledForSubDirs() const { return m_enabledForSubDirs; }
    void setEnabledForSubDirs(bool enabled) { m_enabledForSubDirs = enabled; }

    Utils::Environment makeEnvironment() const;

protected:
    void supportDisablingForSubdirs() { m_disablingForSubDirsSupported = true; }
    virtual QStringList displayArguments() const;

private:
    friend class Internal::MakeStepConfigWidget;

    static int defaultJobCount();
    QStringList jobArguments() const;

    BaseStringListAspect *m_buildTargetsAspect = nullptr;
    QStringList m_availableTargets;
    BaseStringAspect *m_makeCommandAspect = nullptr;
    BaseStringAspect *m_userArgumentsAspect = nullptr;
    BaseIntegerAspect *m_userJobCountAspect = nullptr;
    Internal::OverrideMakeflagsAspect *m_overrideMakeflagsAspect = nullptr;
    BaseBoolAspect *m_cleanAspect = nullptr;
    bool m_disablingForSubDirsSupported = false;
    bool m_enabledForSubDirs = true;
};

} // namespace ProjectExplorer
