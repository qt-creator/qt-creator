// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS
#include "vcpkg_test.h"
#endif // WITH_TESTS
#include "vcpkgmanifesteditor.h"
#include "vcpkgsettings.h"

#include <extensionsystem/iplugin.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

using namespace ProjectExplorer;

namespace Vcpkg::Internal {

class VcpkgPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Vcpkg.json")

public:
    void initialize() final
    {
        setupVcpkgManifestEditor();

#ifdef WITH_TESTS
        addTestCreator(createVcpkgSearchTest);
#endif
    }

    void extensionsInitialized() final
    {
        settings(nullptr).setVcpkgRootEnvironmentVariable();

        connect(
            ProjectManager::instance(),
            &ProjectManager::startupProjectChanged,
            this,
            [](Project *project) { settings(project).setVcpkgRootEnvironmentVariable(); });
    }
};

} // namespace Vcpkg::Internal

#include "vcpkgplugin.moc"
