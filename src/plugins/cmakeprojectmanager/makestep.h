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

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class MakeStepFactory;

class MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class MakeStepFactory;
    friend class MakeStepConfigWidget; // TODO remove
    // This is for modifying internal data

public:
    MakeStep(ProjectExplorer::BuildConfiguration *bc);
    virtual ~MakeStep();

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;

    virtual bool init();

    virtual void run(QFutureInterface<bool> &fi);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    bool buildsBuildTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    QStringList additionalArguments() const;
    void setAdditionalArguments(const QStringList &list);

    void setClean(bool clean);

    QVariantMap toMap() const;

protected:
    MakeStep(ProjectExplorer::BuildConfiguration *bc, MakeStep *bs);
    MakeStep(ProjectExplorer::BuildConfiguration *bc, const QString &id);

    bool fromMap(const QVariantMap &map);

    // For parsing [ 76%]
    virtual void stdOutput(const QString &line);

private:
    void ctor();

    bool m_clean;
    QRegExp m_percentProgress;
    QFutureInterface<bool> *m_futureInterface;
    QStringList m_buildTargets;
    QStringList m_additionalArguments;
};

class MakeStepConfigWidget :public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MakeStepConfigWidget(MakeStep *makeStep);
    virtual QString displayName() const;
    virtual void init();
    virtual QString summaryText() const;
private slots:
    void itemChanged(QListWidgetItem*);
    void additionalArgumentsEdited();
    void updateDetails();
    void buildTargetsChanged();
private:
    MakeStep *m_makeStep;
    QListWidget *m_buildTargetsList;
    QLineEdit *m_additionalArguments;
    QString m_summaryText;
};

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MakeStepFactory(QObject *parent = 0);
    virtual ~MakeStepFactory();

    virtual bool canCreate(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QString &id) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QString &id);
    virtual bool canClone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, ProjectExplorer::BuildStep *source) const;
    virtual ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, ProjectExplorer::BuildStep *source);
    virtual bool canRestore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QVariantMap &map);

    virtual QStringList availableCreationIds(ProjectExplorer::BuildConfiguration *bc, ProjectExplorer::StepType type) const;
    virtual QString displayNameForId(const QString &id) const;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // MAKESTEP_H
