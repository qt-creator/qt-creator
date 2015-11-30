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

#ifndef PROCESSSTEP_H
#define PROCESSSTEP_H

#include "ui_processstep.h"
#include "abstractprocessstep.h"

namespace ProjectExplorer {

namespace Internal {

class ProcessStepFactory : public IBuildStepFactory
{
    Q_OBJECT

public:
    ProcessStepFactory();
    ~ProcessStepFactory();

    virtual QList<Core::Id> availableCreationIds(BuildStepList *parent) const;
    virtual QString displayNameForId(Core::Id id) const;

    virtual bool canCreate(BuildStepList *parent, Core::Id id) const;
    virtual BuildStep *create(BuildStepList *parent, Core::Id id);
    virtual bool canRestore(BuildStepList *parent, const QVariantMap &map) const;
    virtual BuildStep *restore(BuildStepList *parent, const QVariantMap &map);
    virtual bool canClone(BuildStepList *parent, BuildStep *product) const;
    virtual BuildStep *clone(BuildStepList *parent, BuildStep *product);
};

class ProcessStep : public AbstractProcessStep
{
    Q_OBJECT
    friend class ProcessStepFactory;

public:
    explicit ProcessStep(BuildStepList *bsl);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &) override;

    BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    QString command() const;
    QString arguments() const;
    QString workingDirectory() const;

    void setCommand(const QString &command);
    void setArguments(const QString &arguments);
    void setWorkingDirectory(const QString &workingDirectory);

    QVariantMap toMap() const override;

protected:
    ProcessStep(BuildStepList *bsl, ProcessStep *bs);

    bool fromMap(const QVariantMap &map) override;

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
