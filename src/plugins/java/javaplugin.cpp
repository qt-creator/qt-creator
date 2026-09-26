// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaeditor.h"
#include "javalanguageserver.h"

#ifdef WITH_TESTS
#  include "javaclasspath_test.h"
#endif

#include <extensionsystem/iplugin.h>

namespace Java::Internal {

class JavaPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Java.json")

public:
    JavaPlugin() = default;
    ~JavaPlugin() final = default;

    void initialize() final
    {
        setupJavaEditor();
        setupJavaLanguageServer();

#ifdef WITH_TESTS
        addTestCreator(createJavaClassPathTest);
#endif
    }
};

} // namespace Java::Internal

#include "javaplugin.moc"
