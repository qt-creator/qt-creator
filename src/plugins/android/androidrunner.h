// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/QTaskTree>

namespace Android::Internal {

// The application is up once the barrier is passed. A Java debugger attaches to what the
// channel holds by then, if it was given one.
QtTaskTree::Group androidKicker(const QtTaskTree::QStoredBarrier &barrier,
                                ProjectExplorer::RunControl *runControl,
                                const std::shared_ptr<QString> &javaDebugChannel = {});
QtTaskTree::Group androidRecipe(ProjectExplorer::RunControl *runControl);
void setupAndroidRunWorker();

} // namespace Android::Internal
