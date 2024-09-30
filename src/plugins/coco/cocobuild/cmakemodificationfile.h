// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modificationfile.h"

namespace ProjectExplorer {
class Project;
}

namespace Coco::Internal {

class CMakeModificationFile : public ModificationFile
{
public:
    CMakeModificationFile(ProjectExplorer::Project *project);

    void read() override;
    void write() const override;

    QString fileName() const override;
    void setProjectDirectory(const Utils::FilePath &projectDirectory) override;

protected:
    QStringList defaultModificationFile() const override;

private:
    ProjectExplorer::Project *m_project;
};

} // namespace Coco::Internal
