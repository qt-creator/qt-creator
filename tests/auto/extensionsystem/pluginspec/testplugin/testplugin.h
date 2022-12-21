// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testplugin_global.h"
#include <extensionsystem/iplugin.h>

#include <QObject>

namespace MyPlugin {

class MYPLUGIN_EXPORT MyPluginImpl : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "plugin" FILE "testplugin.json")

public:
    MyPluginImpl();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();

public slots:
    bool isInitialized() { return m_isInitialized; }
    bool isExtensionsInitialized() { return m_isExtensionsInitialized; }

private:
    bool m_isInitialized;
    bool m_isExtensionsInitialized;
};

} // namespace
