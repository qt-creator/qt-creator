/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GENERICMAKESTEP_H
#define GENERICMAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace GenericProjectManager {
namespace Internal {

class GenericMakeStepConfigWidget;
class GenericMakeStepFactory;
namespace Ui { class GenericMakeStep; }

class GenericMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class GenericMakeStepConfigWidget;
    friend class GenericMakeStepFactory;

public:
    explicit GenericMakeStep(ProjectExplorer::BuildStepList *parent);

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

    QVariantMap toMap() const override;

protected:
    GenericMakeStep(ProjectExplorer::BuildStepList *parent, GenericMakeStep *bs);
    GenericMakeStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    QStringList m_buildTargets;
    QString m_makeArguments;
    QString m_makeCommand;
    bool m_clean;
};

class GenericMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    GenericMakeStepConfigWidget(GenericMakeStep *makeStep);
    ~GenericMakeStepConfigWidget();
    QString displayName() const;
    QString summaryText() const;

private slots:
    void itemChanged(QListWidgetItem *item);
    void makeLineEditTextEdited();
    void makeArgumentsLineEditTextEdited();
    void updateMakeOverrrideLabel();
    void updateDetails();

private:
    Ui::GenericMakeStep *m_ui;
    GenericMakeStep *m_makeStep;
    QString m_summaryText;
};

class GenericMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit GenericMakeStepFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const;
    QString displayNameForId(Core::Id id) const;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICMAKESTEP_H
