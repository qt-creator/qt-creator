// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace TextEditor { class ICodeStylePreferencesFactory; }

namespace ClangFormat {

class ClangFormatPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ClangFormat.json")

    ~ClangFormatPlugin() override;
    void initialize() final;

    TextEditor::ICodeStylePreferencesFactory *m_factory = nullptr;
};

} // namespace ClangTools
