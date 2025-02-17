// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ProjectExplorer { class BuildConfiguration; }

namespace Coco::Internal {

class BuildSettings;

BuildSettings *createCocoQMakeSettings(ProjectExplorer::BuildConfiguration *bc);

} // namespace Coco::Internal
