// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ProjectExplorer { class RunControl; }
namespace Utils { class FilePath; }

namespace Valgrind::Internal {

bool isPaused();
QString fetchAndResetToggleCollectFunction();
Utils::FilePath remoteOutputFile();
void setupPid(qint64 pid);
void setupRunControl(ProjectExplorer::RunControl *runControl);
void startParser();

void setupCallgrindTool(QObject *guard);

} // Valgrind::Internal
