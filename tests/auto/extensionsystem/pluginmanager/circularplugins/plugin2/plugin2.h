// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#if defined(PLUGIN2_LIBRARY)
#  define PLUGIN2_EXPORT Q_DECL_EXPORT
#elif defined(PLUGIN2_STATIC_LIBRARY)
#  define PLUGIN2_EXPORT
#else
#  define PLUGIN2_EXPORT Q_DECL_IMPORT
#endif

namespace Plugin2 {

class PLUGIN2_EXPORT MyPlugin2 : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "plugin" FILE "plugin2.json")

public:
    MyPlugin2() = default;

    bool initialize(const QStringList &arguments, QString *errorString) final;
};

} // Plugin2
