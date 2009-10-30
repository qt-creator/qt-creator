/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

namespace Internal {
class QMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    QMakeStepFactory();
    virtual ~QMakeStepFactory();
    bool canCreate(const QString & name) const;
    ProjectExplorer::BuildStep * create(ProjectExplorer::Project * pro, const QString & name) const;
    QStringList canCreateForProject(ProjectExplorer::Project *pro) const;
    QString displayNameForName(const QString &name) const;
};
}

class Qt4Project;

class QMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    QMakeStep(Qt4Project * project);
    ~QMakeStep();
    virtual bool init(const QString &name);
    virtual void run(QFutureInterface<bool> &);
    virtual QString name();
    virtual QString displayName();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    QStringList arguments(const QString &buildConfiguration);
    void setForced(bool b);
    bool forced();

    void setQMakeArguments(const QString &buildConfiguraion, const QStringList &arguments);

signals:
    void changed();

protected:
    virtual void processStartupFailed();
    virtual bool processFinished(int exitCode, QProcess::ExitStatus status);

private:
    Qt4Project *m_pro;
    // last values
    QString m_buildConfiguration;
    QStringList m_lastEnv;
    bool m_forced;
};


class QMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QMakeStepConfigWidget(QMakeStep *step);
    QString displayName() const;
    void init(const QString &buildConfiguration);
    QString summaryText() const;
private slots:
    void qmakeArgumentsLineEditTextEdited();
    void buildConfigurationChanged();
    void update();
    void qtVersionChanged(ProjectExplorer::BuildConfiguration *bc);
private:
    void updateTitleLabel();
    void updateEffectiveQMakeCall();
    QString m_buildConfiguration;
    Ui::QMakeStep m_ui;
    QMakeStep *m_step;
    QString m_summaryText;
};

} // namespace Qt4ProjectManager

#endif // QMAKESTEP_H
