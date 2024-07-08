// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagertr.h"

#include "extensionmanagerconstants.h"
#include "extensionmanagerwidget.h"
#ifdef WITH_TESTS
#include "extensionmanager_test.h"
#endif // WITH_TESTS

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>

#include <extensionsystem/iplugin.h>

#include <utils/icon.h>

using namespace Core;
using namespace Utils;

namespace ExtensionManager::Internal {

class ExtensionManagerMode final : public IMode
{
public:
    ExtensionManagerMode()
    {
        setObjectName("ExtensionManagerMode");
        setId(Constants::C_EXTENSIONMANAGER);
        setContext(Context(Constants::MODE_EXTENSIONMANAGER));
        setDisplayName(Tr::tr("Extensions"));
        const Icon FLAT({{":/extensionmanager/images/mode_extensionmanager_mask.png",
                          Theme::IconsBaseColor}});
        const Icon FLAT_ACTIVE({{":/extensionmanager/images/mode_extensionmanager_mask.png",
                                 Theme::IconsModeWelcomeActiveColor}});
        setIcon(Icon::modeIcon(FLAT, FLAT, FLAT_ACTIVE));
        setPriority(72);
        setWidgetCreator(&createExtensionManagerWidget);
    }
};

class ExtensionManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ExtensionManager.json")

public:
    ~ExtensionManagerPlugin() final
    {
        delete m_mode;
    }

    void initialize() final
    {
        m_mode = new ExtensionManagerMode;

#ifdef WITH_TESTS
        addTestCreator(createExtensionsModelTest);
#endif // WITH_TESTS
    }

private:
    ExtensionManagerMode *m_mode = nullptr;
};

} // ExtensionManager::Internal

#include "extensionmanagerplugin.moc"
