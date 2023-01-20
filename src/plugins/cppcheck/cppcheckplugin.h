// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <memory>

namespace Cppcheck::Internal {

class CppcheckPluginPrivate;

class CppcheckPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Cppcheck.json")

public:
    CppcheckPlugin();
    ~CppcheckPlugin() override;

private:
    void initialize() final;

    std::unique_ptr<CppcheckPluginPrivate> d;
};

} // Cppcheck
