// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iplugin.h"

namespace ExtensionSystem {

class PluginSpec;

namespace Internal {

class IPluginPrivate
{
public:
    PluginSpec *pluginSpec;
    QList<std::function<QObject *()>> testCreators;
};

} // namespace Internal
} // namespace ExtensionSystem
