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

QT_FORWARD_DECLARE_CLASS(QListWidgetItem);

namespace GenericProjectManager {
namespace Internal {

class GenericMakeStepConfigWidget;

namespace Ui { class GenericMakeStep; }

class GenericMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class GenericMakeStepConfigWidget;

public:
    explicit GenericMakeStep(ProjectExplorer::BuildStepList *parent, const QString &buildTarget = {});

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    bool buildsTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    QString allArguments() const;
    QString makeCommand(const Utils::Environment &environment) const;

    void setClean(bool clean);
    bool isClean() const;

private:
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;

    QStringList m_buildTargets;
    QString m_makeArguments;
    QString m_makeCommand;
    bool m_clean = false;
};

class GenericMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit GenericMakeStepConfigWidget(GenericMakeStep *makeStep);
    ~GenericMakeStepConfigWidget() override;

    QString displayName() const override;
    QString summaryText() const override;

private:
    void itemChanged(QListWidgetItem *item);
    void makeLineEditTextEdited();
    void makeArgumentsLineEditTextEdited();
    void updateMakeOverrideLabel();
    void updateDetails();

    Ui::GenericMakeStep *m_ui;
    GenericMakeStep *m_makeStep;
    QString m_summaryText;
};

class GenericMakeAllStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    GenericMakeAllStepFactory();
};

class GenericMakeCleanStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    GenericMakeCleanStepFactory();
};

} // namespace Internal
} // namespace GenericProjectManager
