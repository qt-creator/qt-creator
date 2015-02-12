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

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer { class ToolChain; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class MakeStepFactory;

class MakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class MakeStepFactory;

public:
    MakeStep(ProjectExplorer::BuildStepList *bsl);
    virtual ~MakeStep();

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;

    virtual bool init();

    virtual void run(QFutureInterface<bool> &fi);

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;

    QStringList buildTargets() const;
    bool buildsBuildTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    void setBuildTargets(const QStringList &targets);
    void clearBuildTargets();

    QString additionalArguments() const;
    void setAdditionalArguments(const QString &list);

    QString makeCommand(ProjectExplorer::ToolChain *tc, const Utils::Environment &env) const;

    void setClean(bool clean);

    QVariantMap toMap() const;

    void setUserMakeCommand(const QString &make);
    QString userMakeCommand() const;
private:
    void activeBuildConfigurationChanged();
    void buildTargetsChanged();

signals:
    void makeCommandChanged();
    void targetsToBuildChanged();

protected:
    void processStarted();
    void processFinished(int exitCode, QProcess::ExitStatus status);

    MakeStep(ProjectExplorer::BuildStepList *bsl, MakeStep *bs);
    MakeStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);

    bool fromMap(const QVariantMap &map);

    // For parsing [ 76%]
    virtual void stdOutput(const QString &line);

private:
    void ctor();
    CMakeBuildConfiguration *targetsActiveBuildConfiguration() const;

    bool m_clean;
    QRegExp m_percentProgress;
    QRegExp m_ninjaProgress;
    QString m_ninjaProgressString;
    QStringList m_buildTargets;
    QString m_additionalArguments;
    bool m_useNinja;
    CMakeBuildConfiguration *m_activeConfiguration;
    QString m_makeCmd;
};

class MakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MakeStepConfigWidget(MakeStep *makeStep);
    virtual QString displayName() const;
    virtual QString summaryText() const;

private:
    void itemChanged(QListWidgetItem*);
    void makeEdited();
    void additionalArgumentsEdited();
    void updateDetails();
    void buildTargetsChanged();
    void selectedBuildTargetsChanged();

private:
    MakeStep *m_makeStep;
    Utils::PathChooser *m_makePathChooser;
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

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const;
    QString displayNameForId(Core::Id id) const;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // MAKESTEP_H
