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

    NimCompilerBuildStep(ProjectExplorer::BuildStepList *parentList, Utils::Id id);

    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    QStringList userCompilerOptions() const;
    void setUserCompilerOptions(const QStringList &options);

    DefaultBuildOptions defaultCompilerOptions() const;
    void setDefaultCompilerOptions(DefaultBuildOptions options);

    Utils::FilePath targetNimFile() const;
    void setTargetNimFile(const Utils::FilePath &targetNimFile);

    Utils::FilePath outFilePath() const;

signals:
    void userCompilerOptionsChanged(const QStringList &options);
    void defaultCompilerOptionsChanged(DefaultBuildOptions options);
    void targetNimFileChanged(const Utils::FilePath &targetNimFile);
    void processParametersChanged();
    void outFilePathChanged(const Utils::FilePath &outFilePath);

private:
    void setOutFilePath(const Utils::FilePath &outFilePath);

    void updateOutFilePath();
    void updateProcessParameters();
    void updateCommand();
    void updateWorkingDirectory();
    void updateEnvironment();

    void updateTargetNimFile();

    DefaultBuildOptions m_defaultOptions;
    QStringList m_userCompilerOptions;
    Utils::FilePath m_targetNimFile;
    Utils::FilePath m_outFilePath;
};

class NimCompilerBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    NimCompilerBuildStepFactory();
};

} // Nim
