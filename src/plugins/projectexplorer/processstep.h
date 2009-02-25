/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROCESSSTEP_H
#define PROCESSSTEP_H

#include "ui_processstep.h"
#include "abstractprocessstep.h"
#include "environment.h"

namespace ProjectExplorer {

class Project;

namespace Internal {

class ProcessStepFactory : public IBuildStepFactory
{
public:
    ProcessStepFactory();
    virtual bool canCreate(const QString &name) const;
    virtual BuildStep *create(Project *pro, const QString &name) const;
    virtual QStringList canCreateForProject(Project *pro) const;
    virtual QString displayNameForName(const QString &name) const;
};

class ProcessStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    ProcessStep(Project *pro);
    virtual bool init(const QString & name);
    virtual void run(QFutureInterface<bool> &);

    virtual QString name();
    void setDisplayName(const QString &name);
    virtual QString displayName();
    virtual BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
private:
    QString m_name;
};

class ProcessStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    ProcessStepConfigWidget(ProcessStep *step);
    virtual QString displayName() const;
    virtual void init(const QString &buildConfiguration);
private slots:
    void nameLineEditTextEdited();
    void commandLineEditTextEdited();
    void workingDirectoryLineEditTextEdited();
    void commandArgumentsLineEditTextEdited();
    void enabledGroupBoxClicked(bool);
private:
    QString m_buildConfiguration;
    ProcessStep *m_step;
    Ui::ProcessStepWidget m_ui;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROCESSSTEP_H
