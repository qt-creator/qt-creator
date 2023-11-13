// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagertr.h"

#include "extensionmanagerconstants.h"
#include "extensionmanagerwidget.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h> // TODO: Remove!

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginspec.h>

#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/styledbar.h>

#include <QAction>
#include <QMainWindow>

using namespace ExtensionSystem;
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
        setIcon(Utils::Icon::modeIcon(FLAT, FLAT, FLAT_ACTIVE));
        setPriority(72);

        using namespace Layouting;
        auto widget = Column {
            new StyledBar,
            new ExtensionManagerWidget,
            noMargin, spacing(0),
        }.emerge();

        setWidget(widget);
    }

    ~ExtensionManagerMode() { delete widget(); }
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
    }

    bool delayedInitialize() final
    {
        ModeManager::activateMode(m_mode->id()); // TODO: Remove!
        return true;
    }

private:
    ExtensionManagerMode *m_mode = nullptr;
};

} // ExtensionManager::Internal

#include "extensionmanagerplugin.moc"
