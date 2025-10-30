// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qglobal.h>

namespace ProjectExplorer { class RunControl; }

QT_BEGIN_NAMESPACE
namespace QtTaskTree { class Group; }
QT_END_NAMESPACE

namespace QmlProfiler::Internal {

QtTaskTree::Group qmlProfilerRecipe(ProjectExplorer::RunControl *runControl);
QtTaskTree::Group localQmlProfilerRecipe(ProjectExplorer::RunControl *runControl);

void setupQmlProfilerRunning();

} // QmlProfiler::Internal
