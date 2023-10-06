// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/aspects.h>
#include <utils/commandline.h>
#include <utils/guard.h>

#include <QDebug>

QT_BEGIN_NAMESPACE
class QLabel;
class QListWidget;
QT_END_NAMESPACE

namespace ProjectExplorer { class Abi; }

namespace QtSupport { class QtVersion; }

namespace QmakeProjectManager {
class QmakeBuildConfiguration;
class QmakeBuildSystem;

namespace Internal {

class QMakeStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QMakeStepFactory();
};

} // namespace Internal

class QMAKEPROJECTMANAGER_EXPORT QMakeStepConfig
{
public:
    enum OsType { NoOsType, IphoneSimulator, IphoneOS };

    static OsType osTypeFor(const ProjectExplorer::Abi &targetAbi, const QtSupport::QtVersion *version);

    QStringList toArguments() const;

    friend bool operator==(const QMakeStepConfig &a, const QMakeStepConfig &b)
    {
        return a.osType == b.osType
            && a.linkQmlDebuggingQQ2 == b.linkQmlDebuggingQQ2
            && a.useQtQuickCompiler == b.useQtQuickCompiler
            && a.separateDebugInfo == b.separateDebugInfo;
    }

    friend bool operator!=(const QMakeStepConfig &a, const QMakeStepConfig &b) { return !(a == b); }

    friend QDebug operator<<(QDebug dbg, const QMakeStepConfig &c)
    {
        dbg << c.osType
            << (c.linkQmlDebuggingQQ2 == Utils::TriState::Enabled)
            << (c.useQtQuickCompiler == Utils::TriState::Enabled)
            << (c.separateDebugInfo == Utils::TriState::Enabled);
        return dbg;
    }

    QString sysRoot;
    QString targetTriple;
    OsType osType = NoOsType;
    Utils::TriState separateDebugInfo;
    Utils::TriState linkQmlDebuggingQQ2;
    Utils::TriState useQtQuickCompiler;
};

class QMAKEPROJECTMANAGER_EXPORT QMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::QMakeStepFactory;

public:
    QMakeStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);

    QmakeBuildConfiguration *qmakeBuildConfiguration() const;
    QmakeBuildSystem *qmakeBuildSystem() const;
    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QWidget *createConfigWidget() override;
    void setForced(bool b);

    enum class ArgumentFlag {
        Expand = 0x02
    };
    Q_DECLARE_FLAGS(ArgumentFlags, ArgumentFlag);

    // the complete argument line
    QString allArguments(const QtSupport::QtVersion *v,
                         ArgumentFlags flags = ArgumentFlags()) const;
    QMakeStepConfig deducedArguments() const;
    // arguments passed to the pro file parser
    QStringList parserArguments();

    // Extra arguments for qmake and pro file parser. Not user editable via UI.
    QStringList extraArguments() const;
    void setExtraArguments(const QStringList &args);
    /* Extra arguments for pro file parser only. Not user editable via UI.
     * This function is used in 3rd party plugin SailfishOS. */
    QStringList extraParserArguments() const;
    void setExtraParserArguments(const QStringList &args);
    QString mkspec() const;

    Utils::FilePath makeCommand() const;
    QString makeArguments(const QString &makefile) const;
    QString effectiveQMakeCall() const;

    void toMap(Utils::Store &map) const override;

    Utils::SelectionAspect buildType{this};
    ProjectExplorer::ArgumentsAspect userArguments{this};
    Utils::StringAspect effectiveCall{this};

protected:
    void fromMap(const Utils::Store &map) override;

private:
    Tasking::GroupItem runRecipe() final;
    // slots for handling buildconfiguration/step signals
    void qtVersionChanged();
    void qmakeBuildConfigChanged();
    void linkQmlDebuggingLibraryChanged();
    void useQtQuickCompilerChanged();
    void separateDebugInfoChanged();
    void abisChanged();

    // slots for dealing with user changes in our UI
    void qmakeArgumentsLineEdited();
    void buildConfigurationSelected();
    void askForRebuild(const QString &title);

    void recompileMessageBoxFinished(int button);

    void updateAbiWidgets();
    void updateEffectiveQMakeCall();

    Utils::CommandLine m_qmakeCommand;
    Utils::CommandLine m_makeCommand;

    // Extra arguments for qmake and pro file parser
    QStringList m_extraArgs;
    // Extra arguments for pro file parser only
    QStringList m_extraParserArgs;

    // last values
    bool m_forced = false;
    bool m_needToRunQMake = false; // set in init(), read in run()

    bool m_runMakeQmake = false;
    bool m_scriptTemplate = false;
    QStringList m_selectedAbis;
    Utils::OutputFormatter *m_outputFormatter = nullptr;

    Utils::Guard m_ignoreChanges;

    QLabel *abisLabel = nullptr;
    QListWidget *abisListWidget = nullptr;
};

} // namespace QmakeProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmakeProjectManager::QMakeStep::ArgumentFlags);
