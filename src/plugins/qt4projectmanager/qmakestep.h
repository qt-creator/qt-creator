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

#ifndef QMAKESTEP_H
#define QMAKESTEP_H

#include "ui_qmakestep.h"

#include <projectexplorer/abstractprocessstep.h>

#include <QStringList>

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
class Project;
}

namespace Qt4ProjectManager {
class Qt4Project;

namespace Internal {
class Qt4BuildConfiguration;

class QMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit QMakeStepFactory(QObject *parent = 0);
    virtual ~QMakeStepFactory();
    bool canCreate(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type,const QString & id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type,const QString &id);
    bool canClone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type,ProjectExplorer::BuildStep *bs) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type,ProjectExplorer::BuildStep *bs);
    bool canRestore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type,const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type,const QVariantMap &map);
    QStringList availableCreationIds(ProjectExplorer::BuildConfiguration *bc, ProjectExplorer::StepType type) const;
    QString displayNameForId(const QString &id) const;
};

} // namespace Internal


class QMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::QMakeStepFactory;

public:
    explicit QMakeStep(Internal::Qt4BuildConfiguration *parent);
    virtual ~QMakeStep();

    Internal::Qt4BuildConfiguration *qt4BuildConfiguration() const;
    virtual bool init();
    virtual void run(QFutureInterface<bool> &);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    void setForced(bool b);
    bool forced();

    QStringList allArguments();
    QStringList userArguments();
    void setUserArguments(const QStringList &arguments);

    QVariantMap toMap() const;

signals:
    void userArgumentsChanged();

protected:
    QMakeStep(Internal::Qt4BuildConfiguration *parent, QMakeStep *source);
    QMakeStep(Internal::Qt4BuildConfiguration *parent, const QString &id);
    virtual bool fromMap(const QVariantMap &map);

    virtual void processStartupFailed();
    virtual bool processFinished(int exitCode, QProcess::ExitStatus status);

private:
    void ctor();

    // last values
    QStringList m_lastEnv;
    bool m_forced;
    bool m_needToRunQMake; // set in init(), read in run()
    QStringList m_userArgs;
};


class QMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QMakeStepConfigWidget(QMakeStep *step);
    void init();
    QString summaryText() const;
    QString displayName() const;
private slots:
    // slots for handling buildconfiguration/step signals
    void qtVersionChanged();
    void qmakeBuildConfigChanged();
    void userArgumentsChanged();

    // slots for dealing with user changes in our UI
    void qmakeArgumentsLineEdited();
    void buildConfigurationSelected();
private:
    void updateSummaryLabel();
    void updateEffectiveQMakeCall();
    Ui::QMakeStep m_ui;
    QMakeStep *m_step;
    QString m_summaryText;
    bool m_ignoreChange;
};

} // namespace Qt4ProjectManager

#endif // QMAKESTEP_H
