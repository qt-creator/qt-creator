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

protected:
    void supportDisablingForSubdirs() { m_disablingForSubDirsSupported = true; }
    virtual QStringList displayArguments() const;

    Utils::FilePathAspect m_makeCommandAspect{this};
    Utils::MultiSelectionAspect m_buildTargetsAspect{this};
    Utils::StringAspect m_userArgumentsAspect{this};
    Utils::BoolAspect m_overrideMakeflagsAspect{this};
    Utils::TextDisplay m_nonOverrideWarning{this};
    Utils::IntegerAspect m_jobCountAspect{this};
    Utils::BoolAspect m_disabledForSubdirsAspect{this};

private:
    static int defaultJobCount();
    QStringList jobArguments() const;

    bool m_disablingForSubDirsSupported = false;
};

} // namespace ProjectExplorer
