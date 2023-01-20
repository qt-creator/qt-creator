// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace QmakeProjectManager {
namespace Internal {

class QmakeProjectManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmakeProjectManager.json")

public:
    ~QmakeProjectManagerPlugin() final;

#ifdef WITH_TESTS
private slots:
    void testQmakeOutputParsers_data();
    void testQmakeOutputParsers();
    void testMakefileParser_data();
    void testMakefileParser();
#endif

private:
    void initialize() final;

    class QmakeProjectManagerPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace QmakeProjectManager
