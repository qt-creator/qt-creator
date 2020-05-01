/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include "exewrappers/mesonwrapper.h"
#include "mesonprojectimporter.h"

#include "projectexplorer/project.h"
#include "projectexplorer/projectimporter.h"
#include "projectexplorer/task.h"
#include "utils/fileutils.h"
#include <QObject>

namespace MesonProjectManager {
namespace Internal {
class MesonProject final : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    explicit MesonProject(const Utils::FilePath &path);
    ~MesonProject() final = default;

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::ProjectImporter *projectImporter() const final;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;
    ProjectExplorer::MakeInstallCommand makeInstallCommand(const ProjectExplorer::Target *target,
                                                           const QString &installRoot) override;

    mutable std::unique_ptr<MesonProjectImporter> m_projectImporter;
};

} // namespace Internal
} // namespace MesonProjectManager
