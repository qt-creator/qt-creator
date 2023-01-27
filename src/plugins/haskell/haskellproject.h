// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/treescanner.h>

namespace Haskell {
namespace Internal {

class HaskellProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit HaskellProject(const Utils::FilePath &fileName);

    static bool isHaskellProject(Project *project);
};

class HaskellBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    HaskellBuildSystem(ProjectExplorer::Target *t);

    void triggerParsing() override;
    QString name() const final { return QLatin1String("haskell"); }

private:
    void updateApplicationTargets();
    void refresh();

private:
    ParseGuard m_parseGuard;
    ProjectExplorer::TreeScanner m_scanner;
};

} // namespace Internal
} // namespace Haskell
