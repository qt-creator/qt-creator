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

struct ProcessStepSettings
{
    QString command;
    QStringList arguments;
    QString workingDirectory;
    Environment env;
    bool enabled;
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

    virtual void restoreFromMap(const QMap<QString, QVariant> &map);
    virtual void storeIntoMap(QMap<QString, QVariant> &map);

    virtual void restoreFromMap(const QString &buildConfiguration, const QMap<QString, QVariant> &map);
    virtual void storeIntoMap(const QString &buildConfiguration, QMap<QString, QVariant> &map);

    virtual void addBuildConfiguration(const QString & name);
    virtual void removeBuildConfiguration(const QString & name);
    virtual void copyBuildConfiguration(const QString &source, const QString &dest);

    QString command(const QString &buildConfiguration) const;
    QStringList arguments(const QString &buildConfiguration) const;
    bool enabled(const QString &buildConfiguration) const;
    QString workingDirectory(const QString &buildConfiguration) const;

    void setCommand(const QString &buildConfiguration, const QString &command);
    void setArguments(const QString &buildConfiguration, const QStringList &arguments);
    void setEnabled(const QString &buildConfiguration, bool enabled);
    void setWorkingDirectory(const QString &buildConfiguration, const QString &workingDirectory);

private:
    QString m_name;
    QMap<QString, ProcessStepSettings> m_values;
};

class ProcessStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    ProcessStepConfigWidget(ProcessStep *step);
    virtual QString displayName() const;
    virtual void init(const QString &buildConfiguration);
    virtual QString summaryText() const;
private slots:
    void nameLineEditTextEdited();
    void commandLineEditTextEdited();
    void workingDirectoryLineEditTextEdited();
    void commandArgumentsLineEditTextEdited();
    void enabledCheckBoxClicked(bool);
private:
    void updateDetails();
    QString m_buildConfiguration;
    ProcessStep *m_step;
    Ui::ProcessStepWidget m_ui;
    QString m_summaryText;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROCESSSTEP_H
