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

#include "ui_processstep.h"
#include "abstractprocessstep.h"

namespace ProjectExplorer {

namespace Internal {

class ProcessStepFactory : public IBuildStepFactory
{
    Q_OBJECT

public:
    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    BuildStep *create(BuildStepList *parent, Core::Id id) override;
    BuildStep *clone(BuildStepList *parent, BuildStep *product) override;
};

class ProcessStep : public AbstractProcessStep
{
    Q_OBJECT
    friend class ProcessStepFactory;

public:
    explicit ProcessStep(BuildStepList *bsl);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &) override;

    BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    QString command() const;
    QString arguments() const;
    QString workingDirectory() const;

    void setCommand(const QString &command);
    void setArguments(const QString &arguments);
    void setWorkingDirectory(const QString &workingDirectory);

    QVariantMap toMap() const override;

protected:
    ProcessStep(BuildStepList *bsl, ProcessStep *bs);

    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    QString m_command;
    QString m_arguments;
    QString m_workingDirectory;
};

class ProcessStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    ProcessStepConfigWidget(ProcessStep *step);
    virtual QString displayName() const;
    virtual QString summaryText() const;
private:
    void commandLineEditTextEdited();
    void workingDirectoryLineEditTextEdited();
    void commandArgumentsLineEditTextEdited();
    void updateDetails();
    ProcessStep *m_step;
    Ui::ProcessStepWidget m_ui;
    QString m_summaryText;
};

} // namespace Internal
} // namespace ProjectExplorer
