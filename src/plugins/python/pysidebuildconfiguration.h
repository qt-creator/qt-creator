/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>

namespace Python {
namespace Internal {

class PySideBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
public:
    PySideBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
};

class PySideBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
    Q_DECLARE_TR_FUNCTIONS(Python::Internal::PySideBuildConfigurationFactory)
public:
    PySideBuildConfigurationFactory();
};

class PySideBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    PySideBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    void updateInterpreter(const Utils::FilePath &python);

private:
    Utils::StringAspect *m_pysideProject;

private:
    void doRun() override;
};

class PySideBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
    Q_DECLARE_TR_FUNCTIONS(Python::Internal::PySideBuildStepFactory)
public:
    PySideBuildStepFactory();
};

} // namespace Internal
} // namespace Python
