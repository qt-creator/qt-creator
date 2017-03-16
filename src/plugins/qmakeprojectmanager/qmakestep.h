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

#include "qmakeprojectmanager_global.h"
#include <projectexplorer/abstractprocessstep.h>

#include <QStringList>
#include <QFutureWatcher>

namespace Utils { class FileName; }

namespace ProjectExplorer {
class Abi;
class BuildStep;
class IBuildStepFactory;
class Project;
class Kit;
} // namespace ProjectExplorer

namespace QtSupport { class BaseQtVersion; }

namespace QmakeProjectManager {
class QmakeBuildConfiguration;
class QmakeProject;

namespace Internal {

namespace Ui { class QMakeStep; }

class QMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit QMakeStepFactory(QObject *parent = 0);

    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *bs) override;
};

} // namespace Internal

class QMAKEPROJECTMANAGER_EXPORT QMakeStepConfig
{
public:
    enum TargetArchConfig {
        NoArch, X86, X86_64, PowerPC, PowerPC64
    };

    enum OsType {
        NoOsType, IphoneSimulator, IphoneOS
    };

    static TargetArchConfig targetArchFor(const ProjectExplorer::Abi &targetAbi, const QtSupport::BaseQtVersion *version);
    static OsType osTypeFor(const ProjectExplorer::Abi &targetAbi, const QtSupport::BaseQtVersion *version);

    QStringList toArguments() const;

    // Actual data
    TargetArchConfig archConfig = NoArch;
    OsType osType = NoOsType;
    bool linkQmlDebuggingQQ2 = false;
    bool useQtQuickCompiler = false;
    bool separateDebugInfo = false;
};


inline bool operator ==(const QMakeStepConfig &a, const QMakeStepConfig &b) {
    return std::tie(a.archConfig, a.osType, a.linkQmlDebuggingQQ2)
               == std::tie(b.archConfig, b.osType, b.linkQmlDebuggingQQ2)
            && std::tie(a.useQtQuickCompiler, a.separateDebugInfo)
               == std::tie(b.useQtQuickCompiler, b.separateDebugInfo);
}

inline bool operator !=(const QMakeStepConfig &a, const QMakeStepConfig &b) {
    return !(a == b);
}

inline QDebug operator<<(QDebug dbg, const QMakeStepConfig &c)
{
   dbg << c.archConfig << c.osType << c.linkQmlDebuggingQQ2 << c.useQtQuickCompiler << c.separateDebugInfo;
   return dbg;
}

class QMAKEPROJECTMANAGER_EXPORT QMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::QMakeStepFactory;

    // used in DebuggerRunConfigurationAspect
    Q_PROPERTY(bool linkQmlDebuggingLibrary READ linkQmlDebuggingLibrary WRITE setLinkQmlDebuggingLibrary NOTIFY linkQmlDebuggingLibraryChanged)

public:
    explicit QMakeStep(ProjectExplorer::BuildStepList *parent);

    QmakeBuildConfiguration *qmakeBuildConfiguration() const;
    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    void setForced(bool b);
    bool forced();

    // the complete argument line
    QString allArguments(const QtSupport::BaseQtVersion *v, bool shorted = false) const;
    QMakeStepConfig deducedArguments() const;
    // arguments passed to the pro file parser
    QStringList parserArguments();
    // arguments set by the user
    QString userArguments();
    void setUserArguments(const QString &arguments);
    // QMake extra arguments. Not user editable.
    QStringList extraArguments() const;
    void setExtraArguments(const QStringList &args);
    Utils::FileName mkspec() const;
    bool linkQmlDebuggingLibrary() const;
    void setLinkQmlDebuggingLibrary(bool enable);
    bool useQtQuickCompiler() const;
    void setUseQtQuickCompiler(bool enable);
    bool separateDebugInfo() const;
    void setSeparateDebugInfo(bool enable);

    QString makeCommand() const;
    QString makeArguments() const;
    QString effectiveQMakeCall() const;

    QVariantMap toMap() const override;

signals:
    void userArgumentsChanged();
    void extraArgumentsChanged();
    void linkQmlDebuggingLibraryChanged();
    void useQtQuickCompilerChanged();
    void separateDebugInfoChanged();

protected:
    QMakeStep(ProjectExplorer::BuildStepList *parent, QMakeStep *source);
    QMakeStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool fromMap(const QVariantMap &map) override;

    void processStartupFailed() override;
    bool processSucceeded(int exitCode, QProcess::ExitStatus status) override;

private:
    void startOneCommand(const QString &command, const QString &args);
    void runNextCommand();
    void ctor();

    QString m_qmakeExecutable;
    QString m_qmakeArguments;
    QString m_makeExecutable;
    QString m_makeArguments;
    QString m_userArgs;
    // Extra arguments for qmake.
    QStringList m_extraArgs;

    QFutureInterface<bool> m_inputFuture;
    QFutureWatcher<bool> m_inputWatcher;
    std::unique_ptr<QFutureInterface<bool>> m_commandFuture;
    QFutureWatcher<bool> m_commandWatcher;

    // last values
    enum class State { IDLE = 0, RUN_QMAKE, RUN_MAKE_QMAKE_ALL, POST_PROCESS };
    State m_nextState = State::IDLE;
    bool m_forced = false;
    bool m_needToRunQMake = false; // set in init(), read in run()

    bool m_runMakeQmake = false;
    bool m_linkQmlDebuggingLibrary = false;
    bool m_useQtQuickCompiler = false;
    bool m_scriptTemplate = false;
    bool m_separateDebugInfo = false;
};


class QMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QMakeStepConfigWidget(QMakeStep *step);
    ~QMakeStepConfigWidget();
    QString summaryText() const;
    QString additionalSummaryText() const;
    QString displayName() const;
private:
    // slots for handling buildconfiguration/step signals
    void qtVersionChanged();
    void qmakeBuildConfigChanged();
    void userArgumentsChanged();
    void linkQmlDebuggingLibraryChanged();
    void useQtQuickCompilerChanged();
    void separateDebugInfoChanged();

    // slots for dealing with user changes in our UI
    void qmakeArgumentsLineEdited();
    void buildConfigurationSelected();
    void linkQmlDebuggingLibraryChecked(bool checked);
    void useQtQuickCompilerChecked(bool checked);
    void separateDebugInfoChecked(bool checked);
    void askForRebuild();

    void recompileMessageBoxFinished(int button);

    void updateSummaryLabel();
    void updateQmlDebuggingOption();
    void updateQtQuickCompilerOption();
    void updateEffectiveQMakeCall();

    void setSummaryText(const QString &);

    Internal::Ui::QMakeStep *m_ui = nullptr;
    QMakeStep *m_step = nullptr;
    QString m_summaryText;
    QString m_additionalSummaryText;
    bool m_ignoreChange = false;
};

} // namespace QmakeProjectManager
