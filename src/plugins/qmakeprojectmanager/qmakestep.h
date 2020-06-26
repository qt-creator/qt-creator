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
#include <projectexplorer/projectconfigurationaspects.h>

#include <utils/fileutils.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QListWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Abi;
} // namespace ProjectExplorer

namespace QtSupport { class BaseQtVersion; }

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
    QString sysRoot;
    QString targetTriple;
    TargetArchConfig archConfig = NoArch;
    OsType osType = NoOsType;
    ProjectExplorer::TriState separateDebugInfo;
    ProjectExplorer::TriState linkQmlDebuggingQQ2;
    ProjectExplorer::TriState useQtQuickCompiler;
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
   dbg << c.archConfig << c.osType
       << (c.linkQmlDebuggingQQ2 == ProjectExplorer::TriState::Enabled)
       << (c.useQtQuickCompiler == ProjectExplorer::TriState::Enabled)
       << (c.separateDebugInfo == ProjectExplorer::TriState::Enabled);
   return dbg;
}

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
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    void setForced(bool b);

    enum class ArgumentFlag {
        OmitProjectPath = 0x01,
        Expand = 0x02
    };
    Q_DECLARE_FLAGS(ArgumentFlags, ArgumentFlag);

    // the complete argument line
    QString allArguments(const QtSupport::BaseQtVersion *v,
                         ArgumentFlags flags = ArgumentFlags()) const;
    QMakeStepConfig deducedArguments() const;
    // arguments passed to the pro file parser
    QStringList parserArguments();
    // arguments set by the user
    QString userArguments();
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

    QStringList selectedAbis() const;
    void setSelectedAbis(const QStringList &selectedAbis);

signals:
    void userArgumentsChanged();
    void extraArgumentsChanged();

protected:
    bool fromMap(const QVariantMap &map) override;
    void processStartupFailed() override;
    bool processSucceeded(int exitCode, QProcess::ExitStatus status) override;

private:
    void doCancel() override;
    void finish(bool success) override;

    void startOneCommand(const Utils::CommandLine &command);
    void runNextCommand();

    Utils::CommandLine m_qmakeCommand;
    Utils::CommandLine m_makeCommand;
    QString m_userArgs;
    // Extra arguments for qmake and pro file parser
    QStringList m_extraArgs;
    // Extra arguments for pro file parser only
    QStringList m_extraParserArgs;

    // last values
    enum class State { IDLE = 0, RUN_QMAKE, RUN_MAKE_QMAKE_ALL, POST_PROCESS };
    bool m_wasSuccess = true;
    State m_nextState = State::IDLE;
    bool m_forced = false;
    bool m_needToRunQMake = false; // set in init(), read in run()

    bool m_runMakeQmake = false;
    bool m_scriptTemplate = false;
    QStringList m_selectedAbis;
    Utils::OutputFormatter *m_outputFormatter = nullptr;
};


class QMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QMakeStepConfigWidget(QMakeStep *step);
    ~QMakeStepConfigWidget() override;

private:
    // slots for handling buildconfiguration/step signals
    void qtVersionChanged();
    void qmakeBuildConfigChanged();
    void userArgumentsChanged();
    void linkQmlDebuggingLibraryChanged();
    void useQtQuickCompilerChanged();
    void separateDebugInfoChanged();
    void abisChanged();

    // slots for dealing with user changes in our UI
    void qmakeArgumentsLineEdited();
    void buildConfigurationSelected();
    void askForRebuild(const QString &title);

    void recompileMessageBoxFinished(int button);

    void updateSummaryLabel();
    void updateEffectiveQMakeCall();
    bool isAndroidKit() const;

    QMakeStep *m_step = nullptr;
    bool m_ignoreChange = false;

    QLabel *abisLabel = nullptr;
    QComboBox *buildConfigurationComboBox = nullptr;
    QLineEdit *qmakeAdditonalArgumentsLineEdit = nullptr;
    QPlainTextEdit *qmakeArgumentsEdit = nullptr;
    QListWidget *abisListWidget = nullptr;
};

} // namespace QmakeProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmakeProjectManager::QMakeStep::ArgumentFlags);
