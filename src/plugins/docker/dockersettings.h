// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace Docker::Internal {

class DockerSettings final : public Utils::AspectContainer
{
public:
    DockerSettings();

    Utils::StringAspect dockerBinaryPath;
};

class DockerSettingsPage final : public Core::IOptionsPage
{
public:
    explicit DockerSettingsPage(DockerSettings *settings);
};

} // Docker::Internal
