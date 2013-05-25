/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef VCPROJECTMANAGER_INTERNAL_VCMAKESTEP_H
#define VCPROJECTMANAGER_INTERNAL_VCMAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>

class QComboBox;
class QLabel;

namespace VcProjectManager {
namespace Internal {

class VcProjectBuildConfiguration;

class VcMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class VcMakeStepFactory;

public:
    explicit VcMakeStep(ProjectExplorer::BuildStepList *bsl);
    ~VcMakeStep();

    bool init();
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const;

    VcProjectBuildConfiguration *vcProjectBuildConfiguration() const;
    QStringList buildArguments() const;
    QString buildArgumentsToString() const;
    void addBuildArgument(const QString &argument);
    void removeBuildArgument(const QString &buildArgument);

    QVariantMap toMap() const;

protected:
    bool fromMap(const QVariantMap &map);
    void stdOutput(const QString &line);

private:
    explicit VcMakeStep(ProjectExplorer::BuildStepList *parent, VcMakeStep *vcMakeStep);

    QList<ProjectExplorer::Task> m_tasks;
    QFutureInterface<bool> *m_futureInterface;
    ProjectExplorer::ProcessParameters *m_processParams;
    QStringList m_buildArguments;
};

class VcMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    VcMakeStepConfigWidget(VcMakeStep *makeStep);
    QString displayName() const;
    QString summaryText() const;

private slots:
    void msBuildUpdated(); // called when current ms build is chenged in kit information

private:
    VcMakeStep *m_makeStep;
    QLabel *m_msBuildPath;
};

class VcMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit VcMakeStepFactory(QObject *parent = 0);
    ~VcMakeStepFactory();

    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const Core::Id id) const;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCMAKESTEP_H
