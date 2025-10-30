// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/QTaskTree>

namespace Android::Internal {

QtTaskTree::Group androidKicker(const QStoredBarrier &barrier,
                                ProjectExplorer::RunControl *runControl);
QtTaskTree::Group androidRecipe(ProjectExplorer::RunControl *runControl);
void setupAndroidRunWorker();

} // namespace Android::Internal
