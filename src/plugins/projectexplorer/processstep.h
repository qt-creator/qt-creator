/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PROCESSSTEP_H
#define PROCESSSTEP_H

#include "ui_processstep.h"
#include "abstractprocessstep.h"

#include <utils/environment.h>

namespace ProjectExplorer {

namespace Internal {

class ProcessStepFactory : public IBuildStepFactory
{
    Q_OBJECT

public:
    ProcessStepFactory();
    ~ProcessStepFactory();

    virtual QList<Core::Id> availableCreationIds(BuildStepList *parent) const;
    virtual QString displayNameForId(const Core::Id id) const;

    virtual bool canCreate(BuildStepList *parent, const Core::Id id) const;
    virtual BuildStep *create(BuildStepList *parent, const Core::Id id);
    virtual bool canRestore(BuildStepList *parent, const QVariantMap &map) const;
    virtual BuildStep *restore(BuildStepList *parent, const QVariantMap &map);
    virtual bool canClone(BuildStepList *parent, BuildStep *product) const;
    virtual BuildStep *clone(BuildStepList *parent, BuildStep *product);
};

class ProcessStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class ProcessStepFactory;

public:
    explicit ProcessStep(BuildStepList *bsl);
    virtual ~ProcessStep();

    virtual bool init();
    virtual void run(QFutureInterface<bool> &);

    virtual BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;

    QString command() const;
    QString arguments() const;
    QString workingDirectory() const;

    void setCommand(const QString &command);
    void setArguments(const QString &arguments);
    void setWorkingDirectory(const QString &workingDirectory);

    QVariantMap toMap() const;

protected:
    ProcessStep(BuildStepList *bsl, ProcessStep *bs);
    ProcessStep(BuildStepList *bsl, const Core::Id id);

    bool fromMap(const QVariantMap &map);

private:
    void ctor();

    QString m_command;
    QString m_arguments;
    QString m_workingDirectory;
};

class ProcessStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    ProcessStepConfigWidget(ProcessStep *step);
    virtual QString displayName() const;
    virtual QString summaryText() const;
private slots:
    void commandLineEditTextEdited();
    void workingDirectoryLineEditTextEdited();
    void commandArgumentsLineEditTextEdited();
private:
    void updateDetails();
    ProcessStep *m_step;
    Ui::ProcessStepWidget m_ui;
    QString m_summaryText;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROCESSSTEP_H
