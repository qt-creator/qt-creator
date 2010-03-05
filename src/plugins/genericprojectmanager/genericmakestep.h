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

#ifndef GENERICMAKESTEP_H
#define GENERICMAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QListWidgetItem;

namespace Ui {
class GenericMakeStep;
}
QT_END_NAMESPACE

namespace GenericProjectManager {
namespace Internal {

class GenericBuildConfiguration;
class GenericMakeStepConfigWidget;
class GenericMakeStepFactory;

class GenericMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class GenericMakeStepConfigWidget; // TODO remove again?
    friend class GenericMakeStepFactory;

public:
    GenericMakeStep(ProjectExplorer::BuildConfiguration *bc);
    virtual ~GenericMakeStep();

    GenericBuildConfiguration *genericBuildConfiguration() const;

    virtual bool init();

    virtual void run(QFutureInterface<bool> &fi);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    bool buildsTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    QStringList replacedArguments() const;
    QString makeCommand() const;

    QVariantMap toMap() const;

protected:
    GenericMakeStep(ProjectExplorer::BuildConfiguration *bc, GenericMakeStep *bs);
    GenericMakeStep(ProjectExplorer::BuildConfiguration *bc, const QString &id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();

    QStringList m_buildTargets;
    QStringList m_makeArguments;
    QString m_makeCommand;
};

class GenericMakeStepConfigWidget :public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    GenericMakeStepConfigWidget(GenericMakeStep *makeStep);
    virtual QString displayName() const;
    virtual void init();
    virtual QString summaryText() const;
private slots:
    void itemChanged(QListWidgetItem*);
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
    virtual ~GenericMakeStepFactory();

    virtual bool canCreate(ProjectExplorer::BuildConfiguration *parent, const QString &id) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::BuildConfiguration *parent,
                                               const QString &id);
    virtual bool canClone(ProjectExplorer::BuildConfiguration *parent,
                          ProjectExplorer::BuildStep *source) const;
    virtual ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildConfiguration *parent,
                                              ProjectExplorer::BuildStep *source);
    virtual bool canRestore(ProjectExplorer::BuildConfiguration *parent,
                            const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildConfiguration *parent,
                                                const QVariantMap &map);

    virtual QStringList availableCreationIds(ProjectExplorer::BuildConfiguration *bc) const;
    virtual QString displayNameForId(const QString &id) const;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICMAKESTEP_H
