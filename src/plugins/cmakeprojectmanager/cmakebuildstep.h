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

#include <projectexplorer/abstractprocessstep.h>

#include <QRegularExpression>

namespace Utils { class CommandLine; }

namespace ProjectExplorer { class RunConfiguration; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class CMakeBuildStepFactory;

class CMakeBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class CMakeBuildStepFactory;

public:
    CMakeBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;

    QStringList buildTargets() const;
    bool buildsBuildTarget(const QString &target) const;
    void setBuildTargets(const QStringList &target);

    QString cmakeArguments() const;
    void setCMakeArguments(const QString &list);
    QString toolArguments() const;
    void setToolArguments(const QString &list);

    Utils::CommandLine cmakeCommand(ProjectExplorer::RunConfiguration *rc) const;

    QStringList knownBuildTargets();

    QVariantMap toMap() const override;

    static QString cleanTarget();
    static QString allTarget();
    static QString installTarget();
    static QString testTarget();
    static QStringList specialTargets();

signals:
    void targetsToBuildChanged();
    void buildTargetsChanged();

protected:
    void processStarted() override;
    void processFinished(int exitCode, QProcess::ExitStatus status) override;

    bool fromMap(const QVariantMap &map) override;

    // For parsing [ 76%]
    void stdOutput(const QString &output) override;

private:
    void ctor(ProjectExplorer::BuildStepList *bsl);

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    void doRun() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    QString defaultBuildTarget() const;

    void runImpl();
    void handleProjectWasParsed(bool success);

    void handleBuildTargetsChanges(bool success);

    QMetaObject::Connection m_runTrigger;

    QRegularExpression m_percentProgress;
    QRegularExpression m_ninjaProgress;
    QString m_ninjaProgressString;
    QStringList m_buildTargets;
    QString m_cmakeArguments;
    QString m_toolArguments;
    bool m_useNinja = false;
    bool m_waiting = false;
};

class CMakeBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    CMakeBuildStepFactory();
};

} // namespace Internal
} // namespace CMakeProjectManager
