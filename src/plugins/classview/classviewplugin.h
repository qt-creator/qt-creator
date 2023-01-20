// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace ClassView {
namespace Internal {

class ClassViewPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClassView.json")

public:
    ClassViewPlugin() = default;
    ~ClassViewPlugin() final;

private:
    void initialize() final;
    void extensionsInitialized() final {}
};

} // namespace Internal
} // namespace ClassView
