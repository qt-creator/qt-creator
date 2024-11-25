// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>

namespace Nim {

class NimbleProject final : public ProjectExplorer::Project
{
public:
    NimbleProject(const Utils::FilePath &filename);

    // Keep for compatibility with Qt Creator 4.10
    void toMap(Utils::Store &map) const final;

    QStringList excludedFiles() const;
    void setExcludedFiles(const QStringList &excludedFiles);

protected:
    // Keep for compatibility with Qt Creator 4.10
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) final;

    QStringList m_excludedFiles;
};

void setupNimbleProject();

} // Nim
