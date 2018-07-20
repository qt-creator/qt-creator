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

#include <QRegExp>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QRadioButton;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class PathChooser;
} // Utils

namespace ProjectExplorer {
class RunConfiguration;
class ToolChain;
} // ProjectManager

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class CMakeBuildStepFactory;

class CMakeBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class CMakeBuildStepFactory;

public:
    explicit CMakeBuildStep(ProjectExplorer::BuildStepList *bsl);

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;

    QString buildTarget() const;
    bool buildsBuildTarget(const QString &target) const;
    void setBuildTarget(const QString &target);

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
    void cmakeCommandChanged();
    void targetToBuildChanged();
    void buildTargetsChanged();

protected:
    void processStarted() override;
    void processFinished(int exitCode, QProcess::ExitStatus status) override;

    bool fromMap(const QVariantMap &map) override;

    // For parsing [ 76%]
    void stdOutput(const QString &line) override;

private:
    void ctor(ProjectExplorer::BuildStepList *bsl);

    bool init() override;
    void doRun() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    QString defaultBuildTarget() const;

    void runImpl();
    void handleProjectWasParsed(bool success);

    void handleBuildTargetChanges(bool success);

    QMetaObject::Connection m_runTrigger;

    QRegExp m_percentProgress;
    QRegExp m_ninjaProgress;
    QString m_ninjaProgressString;
    QString m_buildTarget;
    QString m_toolArguments;
    bool m_useNinja = false;
    bool m_waiting = false;
};

class CMakeBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    CMakeBuildStepConfigWidget(CMakeBuildStep *buildStep);

private:
    void itemChanged(QListWidgetItem*);
    void toolArgumentsEdited();
    void updateDetails();
    void buildTargetsChanged();
    void updateBuildTarget();

    QRadioButton *itemWidget(QListWidgetItem *item);

    CMakeBuildStep *m_buildStep;
    QLineEdit *m_toolArguments;
    QListWidget *m_buildTargetsList;
};

class CMakeBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    CMakeBuildStepFactory();
};

} // namespace Internal
} // namespace CMakeProjectManager
