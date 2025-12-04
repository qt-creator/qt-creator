// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "buildsystem/qmlbuildsystem.h" // IWYU pragma: keep
#include "qmlprojectmanager_global.h"

#include <extensionsystem/iplugin.h>
#include <projectexplorer/project.h>
#include <utils/expected.h>
#include <utils/filepath.h>

namespace QmlProjectManager {

class QmlProject;

ExtensionSystem::IPlugin *findMcuSupportPlugin();
QMLPROJECTMANAGER_EXPORT Utils::Result<Utils::FilePath> mcuFontsDir();

class QMLPROJECTMANAGER_EXPORT QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    explicit QmlProject(const Utils::FilePath &filename);

    static bool isMCUs();

protected:
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) override;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

    void openStartupQmlFile();
private slots:
    void parsingFinished(bool success);
};

} // namespace QmlProjectManager
