// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractprocessstep.h"

#include <utils/aspects.h>
#include <utils/fileutils.h>

namespace Utils { class Environment; }

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    enum MakeCommandType {
        Display,
        Execution
    };
    explicit MakeStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);

    void setAvailableBuildTargets(const QStringList &buildTargets);
    void setSelectedBuildTarget(const QString &buildTarget);

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QWidget *createConfigWidget() override;

    QStringList availableTargets() const;
    QString userArguments() const;
    void setUserArguments(const QString &args);
    Utils::FilePath makeCommand() const;
    void setMakeCommand(const Utils::FilePath &command);
    Utils::FilePath makeExecutable() const;
    Utils::CommandLine effectiveMakeCommand(MakeCommandType type) const;

    static QString defaultDisplayName();

    Utils::FilePath defaultMakeCommand() const;
    static QString msgNoMakeCommand();
    static Task makeCommandMissingTask();

    virtual bool isJobCountSupported() const;
    bool jobCountOverridesMakeflags() const;
    bool makeflagsContainsJobCount() const;
    bool userArgsContainsJobCount() const;
    bool makeflagsJobCountMismatch() const;

    bool disablingForSubdirsSupported() const { return m_disablingForSubDirsSupported; }
    bool enabledForSubDirs() const;

    Utils::Environment makeEnvironment() const;

    // FIXME: All unused, remove in 4.15.
    void setBuildTarget(const QString &buildTarget) { setSelectedBuildTarget(buildTarget); }
    bool buildsTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);

protected:
    void supportDisablingForSubdirs() { m_disablingForSubDirsSupported = true; }
    virtual QStringList displayArguments() const;

    Utils::StringAspect *makeCommandAspect() const { return m_makeCommandAspect; }
    Utils::MultiSelectionAspect *buildTargetsAspect() const { return m_buildTargetsAspect; }
    Utils::StringAspect *userArgumentsAspect() const { return m_userArgumentsAspect; }
    Utils::BoolAspect *overrideMakeflagsAspect() const { return m_overrideMakeflagsAspect; }
    Utils::TextDisplay *nonOverrideWarning() const { return m_nonOverrideWarning; }
    Utils::IntegerAspect *jobCountAspect() const { return m_userJobCountAspect; }
    Utils::BoolAspect *disabledForSubdirsAspect() const { return m_disabledForSubdirsAspect; }


private:
    static int defaultJobCount();
    QStringList jobArguments() const;

    Utils::MultiSelectionAspect *m_buildTargetsAspect = nullptr;
    QStringList m_availableTargets; // FIXME: Unused, remove in 4.15.
    Utils::StringAspect *m_makeCommandAspect = nullptr;
    Utils::StringAspect *m_userArgumentsAspect = nullptr;
    Utils::IntegerAspect *m_userJobCountAspect = nullptr;
    Utils::BoolAspect *m_overrideMakeflagsAspect = nullptr;
    Utils::BoolAspect *m_disabledForSubdirsAspect = nullptr;
    Utils::TextDisplay *m_nonOverrideWarning = nullptr;
    bool m_disablingForSubDirsSupported = false;
};

} // namespace ProjectExplorer
