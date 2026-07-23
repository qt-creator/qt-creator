// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/icontext.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <utils/id.h>
#include <utils/layoutbuilder.h>

#include <QIcon>
#include <QPointer>
#include <QWidget>

using namespace Core;
using namespace Utils;

namespace Lua::Internal {

void setupModeModule()
{
    registerProvider("Mode", [](sol::state_view lua) -> sol::object {
        const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");
        QObject *guard = pluginSpec->connectionGuard.get();

        sol::table result = lua.create_table();

        // Mode.create{ id, displayName, widget, priority?, icon? } -> handle.
        // Adds a mode (left-side tab with its own full-window widget) and
        // removes it again when the plugin is unloaded.
        result["create"] = [guard](const sol::table &options, sol::this_state s) -> sol::table {
            const QString idString = options.get<QString>("id");
            if (idString.isEmpty())
                throw sol::error("Mode.create: 'id' is required");

            Layouting::Widget *widget = options.get<Layouting::Widget *>("widget");
            if (!widget)
                throw sol::error("Mode.create: 'widget' (a Gui widget) is required");

            IMode *mode = new IMode; // ModeManager::addMode() is called in the ctor.
            const Id id = Id::fromString(idString);
            mode->setId(id);
            mode->setContext(Context(id));
            mode->setDisplayName(options.get_or("displayName", idString));
            mode->setPriority(options.get_or("priority", 0));

            const auto iconPath = options.get<sol::optional<FilePath>>("icon");
            if (iconPath && iconPath->exists())
                mode->setIcon(QIcon(iconPath->toFSPathString()));

            // Hand the widget over through a creator: IMode's destructor deletes
            // a creator-provided widget, so it is cleaned up with the mode.
            QWidget *w = widget->emerge();
            mode->setWidgetCreator([w] { return w; });

            // Take the mode down (tab + registration) on plugin unload.
            QObject::connect(guard, &QObject::destroyed, mode, [mode] {
                ModeManager::removeMode(mode);
                delete mode;
            });

            sol::state_view l(s);
            sol::table handle = l.create_table();
            handle["id"] = idString;
            handle["activate"] = [id] { ModeManager::activateMode(id); };
            const QPointer<IMode> guarded(mode);
            handle["setEnabled"] = [guarded](bool enabled) {
                if (guarded)
                    guarded->setEnabled(enabled);
            };
            return handle;
        };

        return result;
    });
}

} // namespace Lua::Internal
