// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <utils/fileutils.h>

namespace MesonProjectManager {
namespace Internal {

class MachineFileManager final : public QObject
{
    Q_OBJECT
public:
    MachineFileManager();

    static Utils::FilePath machineFile(const ProjectExplorer::Kit *kit);

private:
    void addMachineFile(const ProjectExplorer::Kit *kit);
    void removeMachineFile(const ProjectExplorer::Kit *kit);
    void updateMachineFile(const ProjectExplorer::Kit *kit);
    void cleanupMachineFiles();
};

} // namespace Internal
} // namespace MesonProjectManager
