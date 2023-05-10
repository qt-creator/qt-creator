// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgplugin.h"

#ifdef WITH_TESTS
#include "vcpkg_test.h"
#endif // WITH_TESTS
#include "vcpkgmanifesteditor.h"
#include "vcpkgsettings.h"

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

namespace Vcpkg::Internal {

class VcpkgPluginPrivate
{
public:
    VcpkgManifestEditorFactory manifestEditorFactory;
    VcpkgSettings settings;
};

VcpkgPlugin::~VcpkgPlugin()
{
    delete d;
}

void VcpkgPlugin::initialize()
{
    d = new VcpkgPluginPrivate;
    ProjectExplorer::JsonWizardFactory::addWizardPath(":/vcpkg/wizards/");

#ifdef WITH_TESTS
    addTest<VcpkgSearchTest>();
#endif
}

} // namespace Vcpkg::Internal
