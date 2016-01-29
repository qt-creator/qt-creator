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

#ifndef QMAKESTEP_H
#define QMAKESTEP_H

#include "qmakeprojectmanager_global.h"
#include <projectexplorer/abstractprocessstep.h>

#include <QStringList>

#include <tuple>

namespace Utils { class FileName; }

namespace ProjectExplorer {
class Abi;
class BuildStep;
class IBuildStepFactory;
class Project;
class Kit;
}

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

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *bs) const override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *bs) override;
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const override;
    QString displayNameForId(Core::Id id) const override;
};

} // namespace Internal

class QMAKEPROJECTMANAGER_EXPORT QMakeStepConfig
{
public:
    enum TargetArchConfig {
        NoArch, X86, X86_64, PPC, PPC64
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
    QString allArguments(bool shorted = false);
    QMakeStepConfig deducedArguments();
    // arguments passed to the pro file parser
    QStringList parserArguments();
    // arguments set by the user
    QString userArguments();
    Utils::FileName mkspec();
    void setUserArguments(const QString &arguments);
    bool linkQmlDebuggingLibrary() const;
    void setLinkQmlDebuggingLibrary(bool enable);
    bool useQtQuickCompiler() const;
    void setUseQtQuickCompiler(bool enable);
    bool separateDebugInfo() const;
    void setSeparateDebugInfo(bool enable);

    QVariantMap toMap() const override;

signals:
    void userArgumentsChanged();
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
    void ctor();

    // last values
    bool m_forced = false;
    bool m_needToRunQMake = false; // set in init(), read in run()
    QString m_userArgs;
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
private slots:
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

private slots:
    void recompileMessageBoxFinished(int button);

private:
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

#endif // QMAKESTEP_H
