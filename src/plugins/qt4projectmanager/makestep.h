/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef MAKESTEP_H
#define MAKESTEP_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
}

namespace Qt4ProjectManager {

class Qt4BuildConfiguration;
class MakeStepConfigWidget;

namespace Internal {

namespace Ui { class MakeStep; }

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MakeStepFactory(QObject *parent = 0);
    virtual ~MakeStepFactory();

    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const Core::Id id) const;
};
} //namespace Internal

class Qt4Project;

class QT4PROJECTMANAGER_EXPORT MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::MakeStepFactory;
    friend class MakeStepConfigWidget;

public:
    explicit MakeStep(ProjectExplorer::BuildStepList *bsl);
    virtual ~MakeStep();

    Qt4BuildConfiguration *qt4BuildConfiguration() const;

    virtual bool init();
    virtual void run(QFutureInterface<bool> &);
    bool processSucceeded(int exitCode, QProcess::ExitStatus status);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    QString userArguments();
    void setUserArguments(const QString &arguments);
    void setClean(bool clean);
    bool isClean() const;
    QString makeCommand() const;

    QVariantMap toMap() const;

signals:
    void userArgumentsChanged();

protected:
    MakeStep(ProjectExplorer::BuildStepList *bsl, MakeStep *bs);
    MakeStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    void setMakeCommand(const QString &make);
    bool m_clean;
    bool m_scriptTarget;
    QString m_makeFileToCheck;
    QString m_userArgs;
    QString m_makeCmd;
    QList<ProjectExplorer::Task> m_tasks;
};

class MakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    explicit MakeStepConfigWidget(MakeStep *makeStep);
    virtual ~MakeStepConfigWidget();

    QString displayName() const;
    QString summaryText() const;
private slots:
    // User changes to our widgets
    void makeEdited();
    void makeArgumentsLineEdited();

    void updateDetails();
    void userArgumentsChanged();
    void activeBuildConfigurationChanged();
private:
    void setSummaryText(const QString &text);

    Internal::Ui::MakeStep *m_ui;
    MakeStep *m_makeStep;
    QString m_summaryText;
    ProjectExplorer::BuildConfiguration *m_bc;
    bool m_ignoreChange;
};

} // Qt4ProjectManager

#endif // MAKESTEP_H
