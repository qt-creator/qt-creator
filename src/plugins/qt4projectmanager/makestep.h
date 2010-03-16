/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAKESTEP_H
#define MAKESTEP_H

#include "ui_makestep.h"
#include "qtversionmanager.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/projectexplorer.h>

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
class Project;
}

namespace Qt4ProjectManager {
namespace Internal {
class Qt4BuildConfiguration;

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MakeStepFactory(QObject *parent = 0);
    virtual ~MakeStepFactory();

    bool canCreate(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QString &id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QString &id);
    bool canClone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QVariantMap &map);

    QStringList availableCreationIds(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type) const;
    QString displayNameForId(const QString &id) const;
};
} //namespace Internal

class Qt4Project;

class MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::MakeStepFactory;
    friend class MakeStepConfigWidget; // TODO remove this
    // used to access internal stuff

public:
    explicit MakeStep(ProjectExplorer::BuildConfiguration *bc);
    virtual ~MakeStep();

    Internal::Qt4BuildConfiguration *qt4BuildConfiguration() const;

    virtual bool init();
    virtual void run(QFutureInterface<bool> &);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    QStringList userArguments();
    void setUserArguments(const QStringList &arguments);
    void setClean(bool clean);

    QVariantMap toMap() const;

signals:
    void userArgumentsChanged();

protected:
    MakeStep(ProjectExplorer::BuildConfiguration *bc, MakeStep *bs);
    MakeStep(ProjectExplorer::BuildConfiguration *bc, const QString &id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    bool m_clean;
    QStringList m_userArgs;
    QString m_makeCmd;
};

class MakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MakeStepConfigWidget(MakeStep *makeStep);
    QString displayName() const;
    void init();
    QString summaryText() const;
private slots:
    // User changes to our widgets
    void makeEdited();
    void makeArgumentsLineEdited();

    void updateMakeOverrideLabel();
    void updateDetails();
    void userArgumentsChanged();
private:
    Ui::MakeStep m_ui;
    MakeStep *m_makeStep;
    QString m_summaryText;
    bool m_ignoreChange;
};

} // Qt4ProjectManager

#endif // MAKESTEP_H
