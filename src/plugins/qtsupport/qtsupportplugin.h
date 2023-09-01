// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace QtSupport {
namespace Internal {

class QtSupportPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QtSupport.json")

public:
    ~QtSupportPlugin() final;

private:
    void initialize() final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;

    class QtSupportPluginPrivate *d = nullptr;

#ifdef WITH_TESTS
private slots:
    void testQtOutputParser_data();
    void testQtOutputParser();
    void testQtTestOutputParser();
    void testQtOutputFormatter_data();
    void testQtOutputFormatter();
    void testQtOutputFormatter_appendMessage_data();
    void testQtOutputFormatter_appendMessage();
    void testQtOutputFormatter_appendMixedAssertAndAnsi();

    void testQtProjectImporter_oneProject_data();
    void testQtProjectImporter_oneProject();

    void testQtBuildStringParsing_data();
    void testQtBuildStringParsing();

#if 0
    void testQtProjectImporter_oneProjectExistingKit();
    void testQtProjectImporter_oneProjectNewKitExistingQt();
    void testQtProjectImporter_oneProjectNewKitNewQt();
    void testQtProjectImporter_oneProjectTwoNewKitSameNewQt_pc();
    void testQtProjectImporter_oneProjectTwoNewKitSameNewQt_cp();
    void testQtProjectImporter_oneProjectTwoNewKitSameNewQt_cc();
#endif
#endif
};

} // namespace Internal
} // namespace QtSupport
