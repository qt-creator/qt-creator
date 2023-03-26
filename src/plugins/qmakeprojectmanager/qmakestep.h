// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/abstractprocessstep.h>

#include <utils/aspects.h>
#include <utils/commandline.h>
#include <utils/guard.h>

#include <memory>

#include <QDebug>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QListWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Abi;
class ArgumentsAspect;
} // namespace ProjectExplorer

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
    // TODO remove, does nothing
    enum TargetArchConfig { NoArch, X86, X86_64, PowerPC, PowerPC64 };

    enum OsType { NoOsType, IphoneSimulator, IphoneOS };

    // TODO remove, does nothing
    static TargetArchConfig targetArchFor(const ProjectExplorer::Abi &targetAbi,
                                          const QtSupport::QtVersion *version);
    static OsType osTypeFor(const ProjectExplorer::Abi &targetAbi, const QtSupport::QtVersion *version);

    QStringList toArguments() const;

    friend bool operator==(const QMakeStepConfig &a, const QMakeStepConfig &b)
    {
        return std::tie(a.archConfig, a.osType, a.linkQmlDebuggingQQ2)
                == std::tie(b.archConfig, b.osType, b.linkQmlDebuggingQQ2)
                && std::tie(a.useQtQuickCompiler, a.separateDebugInfo)
                == std::tie(b.useQtQuickCompiler, b.separateDebugInfo);
    }

    friend bool operator!=(const QMakeStepConfig &a, const QMakeStepConfig &b) { return !(a == b); }

    friend QDebug operator<<(QDebug dbg, const QMakeStepConfig &c)
    {
        dbg << c.archConfig << c.osType
            << (c.linkQmlDebuggingQQ2 == Utils::TriState::Enabled)
            << (c.useQtQuickCompiler == Utils::TriState::Enabled)
            << (c.separateDebugInfo == Utils::TriState::Enabled);
        return dbg;
    }

    // Actual data
    QString sysRoot;
    QString targetTriple;
    // TODO remove, does nothing
    TargetArchConfig archConfig = NoArch;
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
    void doRun() override;
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
    // arguments set by the user
    QString userArguments() const;
    void setUserArguments(const QString &arguments);
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

    QVariantMap toMap() const override;

protected:
    bool fromMap(const QVariantMap &map) override;

private:
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
    ProjectExplorer::ArgumentsAspect *m_userArgs = nullptr;
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
    Utils::SelectionAspect *m_buildType = nullptr;
    Utils::StringAspect *m_effectiveCall = nullptr;
    QListWidget *abisListWidget = nullptr;
};

} // namespace QmakeProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmakeProjectManager::QMakeStep::ArgumentFlags);
