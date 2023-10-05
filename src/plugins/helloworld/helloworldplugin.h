// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace HelloWorld::Internal {

class HelloMode;

class HelloWorldPlugin
  : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "HelloWorld.json")

public:
    HelloWorldPlugin();
    ~HelloWorldPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;

private:
    void sayHelloWorld();

    HelloMode *m_helloMode = nullptr;
};

} // namespace HelloWorld::Internal
