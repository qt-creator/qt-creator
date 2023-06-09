// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace ProjectExplorer { class Kit; }

namespace MesonProjectManager::Internal {

class MachineFileManager final : public QObject
{
public:
    MachineFileManager();

    static Utils::FilePath machineFile(const ProjectExplorer::Kit *kit);

private:
    void addMachineFile(const ProjectExplorer::Kit *kit);
    void removeMachineFile(const ProjectExplorer::Kit *kit);
    void updateMachineFile(const ProjectExplorer::Kit *kit);
    void cleanupMachineFiles();
};

} // MesonProjectManager::Internal
