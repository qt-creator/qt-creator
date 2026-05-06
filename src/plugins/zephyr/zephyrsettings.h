// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Zephyr::Internal {

class ZephyrSettings : public Utils::AspectContainer
{
public:
    ZephyrSettings();

    Utils::FilePathAspect westFilePath{this};
    Utils::FilePathAspect workspaceDir{this};
    Utils::FilePathAspect qmlProjectExporterFilePath{this};
};

ZephyrSettings &settings();

} // namespace Zephyr::Internal
