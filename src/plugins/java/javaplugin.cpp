// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaeditor.h"
#include "javalanguageserver.h"
#include "jdkmanager.h"
#include "jdkoptionspage.h"

#ifdef WITH_TESTS
#  include "javaclasspath_test.h"
#  include "javakitaspect_test.h"
#  include "javaparser.h"
#  include "jdkmanager_test.h"
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
        setupJdkOptionsPage();

#ifdef WITH_TESTS
        addTestCreator(createJavaClassPathTest);
        addTestCreator(createJavaKitAspectTest);
        addTestCreator(createJavaParserTest);
        addTestCreator(createJdkManagerTest);
        addTestCreator(createJdkOptionsPageTest);
#endif
    }

    // Here rather than in initialize(), since the detection asks the plugins
    // that know of a JDK of their own and those add themselves while they are
    // initializing - which, depending on this plugin, they do after it.
    // Looks on another thread; the kits do not wait for it.
    void extensionsInitialized() final { JdkManager::restoreJdks(); }
};

} // namespace Java::Internal

#include "javaplugin.moc"
