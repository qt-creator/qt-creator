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

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Ios {
namespace Internal {

class IosBuildStepConfigWidget;
class IosBuildStepFactory;
namespace Ui { class IosBuildStep; }

class IosBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class IosBuildStepConfigWidget;
    friend class IosBuildStepFactory;

public:
    explicit IosBuildStep(ProjectExplorer::BuildStepList *parent);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    void setBaseArguments(const QStringList &args);
    void setExtraArguments(const QStringList &extraArgs);
    QStringList baseArguments() const;
    QStringList allArguments() const;
    QStringList defaultArguments() const;
    QString buildCommand() const;

    void setClean(bool clean);
    bool isClean() const;

    QVariantMap toMap() const override;
protected:
    IosBuildStep(ProjectExplorer::BuildStepList *parent, IosBuildStep *bs);
    IosBuildStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    QStringList m_baseBuildArguments;
    QStringList m_extraArguments;
    QString m_buildCommand;
    bool m_useDefaultArguments;
    bool m_clean;
};

class IosBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    IosBuildStepConfigWidget(IosBuildStep *buildStep);
    ~IosBuildStepConfigWidget();
    QString displayName() const override;
    QString summaryText() const override;

private:
    void buildArgumentsChanged();
    void resetDefaultArguments();
    void extraArgumentsChanged();
    void updateDetails();

    Ui::IosBuildStep *m_ui;
    IosBuildStep *m_buildStep;
    QString m_summaryText;
};

class IosBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit IosBuildStepFactory(QObject *parent = 0);

    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source) override;
};

} // namespace Internal
} // namespace Ios
