// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QDebug>
#include <QGuiApplication>
#include <QIcon>
#include <QPixmap>
#include <QPointer>
#include <QScreen>
#include <QWidget>

#include <algorithm>
#include <cmath>

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

            // The mode bar is much darker than a menu, so a plain (e.g. black)
            // icon would vanish there. Tint the icon to the theme's icon color
            // while preserving its alpha so it stays visible and follows the
            // theme. This is themed by default, unlike the opt-in icon tinting
            // elsewhere in the Lua API (see meta/mode.lua). Rendering goes
            // through QIcon's icon engine (not QPixmap::load, which cannot
            // rasterize a transparent SVG) at the mode-bar size, one pixmap per
            // device pixel ratio.
            const auto iconPath = options.get<sol::optional<FilePath>>("icon");
            if (iconPath && iconPath->exists()) {
                const QString iconFile = iconPath->toFSPathString();
                const QColor tint = creatorColor(Theme::IconsBaseColor);
                const QSize size(Core::Constants::MODEBAR_ICON_SIZE,
                                 Core::Constants::MODEBAR_ICON_SIZE);
                int maxDpr = 1;
                for (const QScreen *screen : QGuiApplication::screens())
                    maxDpr = std::max(maxDpr, int(std::ceil(screen->devicePixelRatio())));
                QIcon themedIcon;
                for (int dpr = 1; dpr <= maxDpr; ++dpr) {
                    const QPixmap src = QIcon(iconFile).pixmap(size, dpr);
                    if (!src.isNull())
                        themedIcon.addPixmap(StyleHelper::tintedPixmap(src, tint));
                }
                if (themedIcon.isNull())
                    qWarning() << "Mode.create: could not load icon" << iconFile;
                else
                    mode->setIcon(themedIcon);
            }

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
