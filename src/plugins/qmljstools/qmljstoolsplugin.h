// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace QmlJSTools {
namespace Internal {

class QmlJSToolsPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlJSTools.json")

public:
    ~QmlJSToolsPlugin() final;

private:
    void initialize() final;
    void extensionsInitialized() final;

    class QmlJSToolsPluginPrivate *d = nullptr;

#ifdef WITH_TESTS
private slots:
    void test_basic();
#endif
};

} // namespace Internal
} // namespace QmlJSTools
