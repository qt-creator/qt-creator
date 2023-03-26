// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclientoutline.h"
#include "callhierarchy.h"

#include <extensionsystem/iplugin.h>

namespace LanguageClient {

class LanguageClientPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "LanguageClient.json")
public:
    LanguageClientPlugin();
    ~LanguageClientPlugin() override;

    static LanguageClientPlugin *instance();

    // IPlugin interface
private:
    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    LanguageClientOutlineWidgetFactory m_outlineFactory;
    CallHierarchyFactory m_callHierarchyFactory;

#ifdef WITH_TESTS
private slots:
    void testSnippetParsing_data();
    void testSnippetParsing();
#endif
};

} // namespace LanguageClient
