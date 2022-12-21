// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Plugin3 {

class MyPlugin3 : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "plugin" FILE "plugin3.json")

public:
    MyPlugin3() = default;
    ~MyPlugin3();

    bool initialize(const QStringList &arguments, QString *errorString) final;
    void extensionsInitialized() final;

private:
    bool initializeCalled = false;
    QObject *object1 = nullptr;
    QObject *object2 = nullptr;
};

} // namespace Plugin3
