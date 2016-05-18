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

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
class Task;
}

namespace QmakeProjectManager {

class QmakeBuildConfiguration;
class MakeStepConfigWidget;

namespace Internal {

namespace Ui { class MakeStep; }

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MakeStepFactory(QObject *parent = 0);

    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
};

} //namespace Internal

class QmakeProject;

class QMAKEPROJECTMANAGER_EXPORT MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::MakeStepFactory;
    friend class MakeStepConfigWidget;

public:
    explicit MakeStep(ProjectExplorer::BuildStepList *bsl);

    QmakeBuildConfiguration *qmakeBuildConfiguration() const;

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    QString userArguments();
    void setUserArguments(const QString &arguments);
    void setClean(bool clean);
    bool isClean() const;
    QString makeCommand() const;

    QString effectiveMakeCommand() const;

    QVariantMap toMap() const override;

signals:
    void userArgumentsChanged();

protected:
    MakeStep(ProjectExplorer::BuildStepList *bsl, MakeStep *bs);
    MakeStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();
    void setMakeCommand(const QString &make);
    QStringList automaticallyAddedArguments() const;
    bool m_clean = false;
    bool m_scriptTarget = false;
    QString m_makeFileToCheck;
    QString m_userArgs;
    QString m_makeCmd;
};

class MakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    explicit MakeStepConfigWidget(MakeStep *makeStep);
    virtual ~MakeStepConfigWidget();

    QString displayName() const;
    QString summaryText() const;
private:
    // User changes to our widgets
    void makeEdited();
    void makeArgumentsLineEdited();

    void updateDetails();
    void userArgumentsChanged();
    void activeBuildConfigurationChanged();
    void setSummaryText(const QString &text);

    Internal::Ui::MakeStep *m_ui = nullptr;
    MakeStep *m_makeStep = nullptr;
    QString m_summaryText;
    ProjectExplorer::BuildConfiguration *m_bc = nullptr;
    bool m_ignoreChange = false;
};

} // QmakeProjectManager
