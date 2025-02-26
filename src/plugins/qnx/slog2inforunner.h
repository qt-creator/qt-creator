// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ProjectExplorer { class RunControl; }
namespace Tasking { class Group; }

namespace Qnx::Internal {

Tasking::Group slog2InfoRecipe(ProjectExplorer::RunControl *runControl);

} // Qnx::Internal
