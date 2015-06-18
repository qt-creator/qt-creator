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

#ifndef MAKESTEP_H
#define MAKESTEP_H

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/abstractprocessstep.h>

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
class Task;
}

namespace QmakeProjectManager {

class QmakeBuildConfiguration;
class MakeStepConfigWidget;

namespace Internal {

namespace Ui { class MakeStep; }

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MakeStepFactory(QObject *parent = 0);
    virtual ~MakeStepFactory();

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(Core::Id id) const;
};
} //namespace Internal

class QmakeProject;

class QMAKEPROJECTMANAGER_EXPORT MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class Internal::MakeStepFactory;
    friend class MakeStepConfigWidget;

public:
    explicit MakeStep(ProjectExplorer::BuildStepList *bsl);
    virtual ~MakeStep();

    QmakeBuildConfiguration *qmakeBuildConfiguration() const;

    virtual bool init();
    virtual void run(QFutureInterface<bool> &);

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
    MakeStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    void setMakeCommand(const QString &make);
    QStringList automaticallyAddedArguments() const;
    bool m_clean = false;
    bool m_scriptTarget = false;
    QString m_makeFileToCheck;
    QString m_userArgs;
    QString m_makeCmd;
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

    Internal::Ui::MakeStep *m_ui = nullptr;
    MakeStep *m_makeStep = nullptr;
    QString m_summaryText;
    ProjectExplorer::BuildConfiguration *m_bc = nullptr;
    bool m_ignoreChange = false;
};

} // QmakeProjectManager

#endif // MAKESTEP_H
