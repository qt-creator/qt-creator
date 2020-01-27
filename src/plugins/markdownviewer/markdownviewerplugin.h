// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Markdown {
namespace Internal {

class MarkdownViewerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "MarkdownViewer.json")

public:
    MarkdownViewerPlugin() = default;
    ~MarkdownViewerPlugin() = default;

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final {}
};

} // namespace Internal
} // namespace Markdown
