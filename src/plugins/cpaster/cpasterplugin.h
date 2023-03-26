// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "codepasterservice.h"

#include <extensionsystem/iplugin.h>

namespace CodePaster {

class CodePasterPluginPrivate;

class CodePasterServiceImpl final : public QObject, public CodePaster::Service
{
    Q_OBJECT
    Q_INTERFACES(CodePaster::Service)

public:
    explicit CodePasterServiceImpl(CodePasterPluginPrivate *d);

private:
    void postText(const QString &text, const QString &mimeType) final;
    void postCurrentEditor() final;
    void postClipboard() final;

    CodePasterPluginPrivate *d = nullptr;
};

class CodePasterPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CodePaster.json")

public:
    enum PasteSource {
        PasteEditor = 0x1,
        PasteClipboard = 0x2
    };
    Q_DECLARE_FLAGS(PasteSources, PasteSource)

    CodePasterPlugin() = default;
    ~CodePasterPlugin() final;

private:
    void initialize() final;
    ShutdownFlag aboutToShutdown() final;

    CodePasterPluginPrivate *d = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CodePasterPlugin::PasteSources)

} // namespace CodePaster
