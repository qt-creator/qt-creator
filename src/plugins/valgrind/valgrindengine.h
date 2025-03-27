// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <solutions/tasking/tasktree.h>

namespace ProjectExplorer { class RunControl; }
namespace Utils { class CommandLine; }

namespace Valgrind::Internal {

class ValgrindProcess;
class ValgrindSettings;

Tasking::ExecutableItem initValgrindRecipe(const Tasking::Storage<ValgrindSettings> &storage,
                                           ProjectExplorer::RunControl *runControl);
void setupValgrindProcess(ValgrindProcess *process, ProjectExplorer::RunControl *runControl,
                          const Utils::CommandLine &valgrindCommand);
Utils::CommandLine defaultValgrindCommand(ProjectExplorer::RunControl *runControl,
                                          const ValgrindSettings &settings);

} // Valgrind::Internal
