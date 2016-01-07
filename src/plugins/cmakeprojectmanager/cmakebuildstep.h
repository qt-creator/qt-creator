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

#ifndef CMAKEBUILDSTEP_H
#define CMAKEBUILDSTEP_H

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
class CMakeRunConfiguration;
class CMakeBuildStepFactory;

class CMakeBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class CMakeBuildStepFactory;

public:
    explicit CMakeBuildStep(ProjectExplorer::BuildStepList *bsl);

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;
    CMakeBuildConfiguration *targetsActiveBuildConfiguration() const;

    bool init(QList<const BuildStep *> &earlierSteps) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    QStringList buildTargets() const;
    bool buildsBuildTarget(const QString &target) const;
    void setBuildTarget(const QString &target, bool on);
    void setBuildTargets(const QStringList &targets);
    void clearBuildTargets();

    QString toolArguments() const;
    void setToolArguments(const QString &list);

    QString allArguments(const CMakeRunConfiguration *rc) const;

    bool addRunConfigurationArgument() const;
    void setAddRunConfigurationArgument(bool add);

    QString cmakeCommand() const;

    QVariantMap toMap() const override;

    static QString cleanTarget();

private:
    void buildTargetsChanged();

signals:
    void cmakeCommandChanged();
    void targetsToBuildChanged();

protected:
    void processStarted() override;
    void processFinished(int exitCode, QProcess::ExitStatus status) override;

    CMakeBuildStep(ProjectExplorer::BuildStepList *bsl, CMakeBuildStep *bs);
    CMakeBuildStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);

    bool fromMap(const QVariantMap &map) override;

    // For parsing [ 76%]
    virtual void stdOutput(const QString &line) override;

private:
    void ctor();
    CMakeRunConfiguration *targetsActiveRunConfiguration() const;

    QRegExp m_percentProgress;
    QRegExp m_ninjaProgress;
    QString m_ninjaProgressString;
    QStringList m_buildTargets;
    QString m_toolArguments;
    bool m_addRunConfigurationArgument;
    bool m_useNinja;
};

class CMakeBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    CMakeBuildStepConfigWidget(CMakeBuildStep *buildStep);
    virtual QString displayName() const;
    virtual QString summaryText() const;

private:
    void itemChanged(QListWidgetItem*);
    void toolArgumentsEdited();
    void updateDetails();
    void buildTargetsChanged();
    void selectedBuildTargetsChanged();

private:
    CMakeBuildStep *m_buildStep;
    QListWidget *m_buildTargetsList;
    QLineEdit *m_toolArguments;
    QString m_summaryText;
};

class CMakeBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit CMakeBuildStepFactory(QObject *parent = 0);

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

#endif // CMAKEBUILDSTEP_H
