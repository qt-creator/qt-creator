// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Docker::Internal {

class DockerSettings final : public Core::PagedSettings
{
public:
    DockerSettings();

    Utils::FilePathAspect dockerBinaryPath{this};
};

} // Docker::Internal
