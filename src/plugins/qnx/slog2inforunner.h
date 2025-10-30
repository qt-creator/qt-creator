// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qglobal.h>

namespace ProjectExplorer { class RunControl; }

QT_BEGIN_NAMESPACE
namespace QtTaskTree { class Group; }
QT_END_NAMESPACE

namespace Qnx::Internal {

QtTaskTree::Group slog2InfoRecipe(ProjectExplorer::RunControl *runControl);

} // Qnx::Internal
