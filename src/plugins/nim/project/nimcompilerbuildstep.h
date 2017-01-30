/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>

namespace Nim {

class NimCompilerBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    enum DefaultBuildOptions { Empty = 0, Debug, Release};

    NimCompilerBuildStep(ProjectExplorer::BuildStepList *parentList);

    bool init(QList<const ProjectExplorer::BuildStep *> &earlierSteps) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    QStringList userCompilerOptions() const;
    void setUserCompilerOptions(const QStringList &options);

    DefaultBuildOptions defaultCompilerOptions() const;
    void setDefaultCompilerOptions(DefaultBuildOptions options);

    Utils::FileName targetNimFile() const;
    void setTargetNimFile(const Utils::FileName &targetNimFile);

    Utils::FileName outFilePath() const;

signals:
    void userCompilerOptionsChanged(const QStringList &options);
    void defaultCompilerOptionsChanged(DefaultBuildOptions options);
    void targetNimFileChanged(const Utils::FileName &targetNimFile);
    void processParametersChanged();
    void outFilePathChanged(const Utils::FileName &outFilePath);

private:
    void setOutFilePath(const Utils::FileName &outFilePath);

    void updateOutFilePath();
    void updateProcessParameters();
    void updateCommand();
    void updateWorkingDirectory();
    void updateArguments();
    void updateEnvironment();

    void updateTargetNimFile();

    DefaultBuildOptions m_defaultOptions;
    QStringList m_userCompilerOptions;
    Utils::FileName m_targetNimFile;
    Utils::FileName m_outFilePath;
};

}
