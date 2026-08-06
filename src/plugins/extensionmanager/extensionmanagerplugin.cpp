// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagertr.h"

#include "extensionmanagerconstants.h"
#include "extensionmanagerwidget.h"
#ifdef WITH_TESTS
#include "extensionmanager_test.h"
#endif // WITH_TESTS

#include <coreplugin/dialogs/ioptionspage.h>

#include <extensionsystem/iplugin.h>

using namespace Core;

namespace ExtensionManager::Internal {

class ExtensionManagerBrowserPage final : public IOptionsPage
{
public:
    ExtensionManagerBrowserPage()
    {
        setId(Constants::EXTENSIONMANAGER_BROWSER_PAGE_ID);
        setDisplayName(Tr::tr("Browse"));
        setCategory(Constants::EXTENSIONMANAGER_SETTINGSPAGE_CATEGORY);
        setWidgetCreator(&createExtensionManagerWidget);
    }
};

class ExtensionManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ExtensionManager.json")

public:
    void initialize() final
    {
        IOptionsPage::registerCategory(
            Constants::EXTENSIONMANAGER_SETTINGSPAGE_CATEGORY,
            Tr::tr("Extensions"),
            ":/extensionmanager/images/settingscategory_extensionmanager.png");

#ifdef WITH_TESTS
        addTestCreator(createExtensionsModelTest);
#endif // WITH_TESTS
    }

private:
    const ExtensionManagerBrowserPage m_browserPage;
};

} // ExtensionManager::Internal

#include "extensionmanagerplugin.moc"
