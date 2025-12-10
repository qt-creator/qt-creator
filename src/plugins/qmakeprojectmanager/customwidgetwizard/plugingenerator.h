// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/generatedfile.h>

namespace QmakeProjectManager::Internal {

struct PluginOptions;

struct GenerationParameters
{
    QString path;
    QString fileName;
    QString templatePath;
};

Utils::Result<Core::GeneratedFiles> generatePlugin(const GenerationParameters &p,
                                                   const PluginOptions &options);

} // QmakeProjectManager::Internal
