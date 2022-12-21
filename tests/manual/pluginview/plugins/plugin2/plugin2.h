// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Plugin2 {

class MyPlugin2 final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "plugin" FILE "plugin2.json")

public:
    MyPlugin2() = default;
    ~MyPlugin2() final;

    bool initialize(const QStringList &arguments, QString *errorString) final;
    void extensionsInitialized() final;

private:
    bool initializeCalled = false;
    QObject *object1 = nullptr;
    QObject *object2 = nullptr;
};

} // namespace Plugin2
