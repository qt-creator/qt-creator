/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <android/androidbuildapkstep.h>

namespace QmakeAndroidSupport {
namespace Internal {

class QmakeAndroidBuildApkStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit QmakeAndroidBuildApkStepFactory(QObject *parent = 0);

    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *product) override;
};

class QmakeAndroidBuildApkStep : public Android::AndroidBuildApkStep
{
    Q_OBJECT
public:
    QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc);
    Utils::FileName proFilePathForInputFile() const;
    void setProFilePathForInputFile(const QString &path);

protected:
    friend class QmakeAndroidBuildApkStepFactory;
    QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc, QmakeAndroidBuildApkStep *other);

    Utils::FileName androidPackageSourceDir() const override;

protected:
    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    void processStarted() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

private:
    void setupProcessParameters(ProjectExplorer::ProcessParameters *pp,
                                ProjectExplorer::BuildConfiguration *bc,
                                const QStringList &arguments, const QString &command);
    QString m_command;
    QString m_argumentsPasswordConcealed;
    bool m_skipBuilding = false;
};

} // namespace Internal
} // namespace QmakeAndroidSupport
