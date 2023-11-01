// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
#include <utils/wizard.h>

namespace ProjectExplorer { class BuildSystem; }

namespace Android {
namespace Internal {

class CreateAndroidManifestWizard : public Utils::Wizard
{
public:
    CreateAndroidManifestWizard(ProjectExplorer::BuildSystem *buildSystem);

    QString buildKey() const;
    void setBuildKey(const QString &buildKey);

    void accept();
    bool copyGradle();

    void setDirectory(const Utils::FilePath &directory);
    void setCopyGradle(bool copy);

    ProjectExplorer::BuildSystem *buildSystem() const;

private:
    void createAndroidManifestFile();
    void createAndroidTemplateFiles();
    ProjectExplorer::BuildSystem *m_buildSystem;
    QString m_buildKey;
    Utils::FilePath m_directory;
    bool m_copyGradle;
};

} // namespace Internal
} // namespace Android
