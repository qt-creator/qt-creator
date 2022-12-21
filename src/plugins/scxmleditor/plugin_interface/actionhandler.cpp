// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "actionhandler.h"
#include "mytypes.h"
#include "scxmleditortr.h"

#include <utils/utilsicons.h>

#include <QAction>

using namespace ScxmlEditor::PluginInterface;

ActionHandler::ActionHandler(QObject *parent)
    : QObject(parent)
{
    const struct {
        const Utils::Icon icon;
        QString name;
        QString tooltip;
        const char *keyseq;
        bool checkable;
    } actionInfos[] = {
        {Utils::Icons::ZOOMIN_TOOLBAR, Tr::tr("Zoom In"), Tr::tr("Zoom In (Ctrl + + / Ctrl + Wheel)"), "Ctrl++", false},
        {Utils::Icons::ZOOMOUT_TOOLBAR, Tr::tr("Zoom Out"), Tr::tr("Zoom Out (Ctrl + - / Ctrl + Wheel)"), "Ctrl+-", false},
        {Utils::Icons::FITTOVIEW_TOOLBAR, Tr::tr("Fit to View"), Tr::tr("Fit to View (F11)"), "F11", false},
        {Utils::Icons::PAN_TOOLBAR, Tr::tr("Panning"), Tr::tr("Panning (Shift)"), "Shift", true},

        {Utils::Icons::ZOOM_TOOLBAR, Tr::tr("Magnifier"), Tr::tr("Magnifier Tool"), "", true},
        {Utils::Icon(":/scxmleditor/images/navigator.png"), Tr::tr("Navigator"), Tr::tr("Navigator (Ctrl+E)"), "Ctrl+E", true},

        {Utils::Icons::COPY_TOOLBAR, Tr::tr("Copy"), Tr::tr("Copy (Ctrl + C)"), "Ctrl+C", false},
        {Utils::Icons::CUT_TOOLBAR, Tr::tr("Cut"), Tr::tr("Cut (Ctrl + X)"), "Ctrl+X", false},
        {Utils::Icons::PASTE_TOOLBAR, Tr::tr("Paste"), Tr::tr("Paste (Ctrl + V)"), "Ctrl+V", false},
        {Utils::Icons::SNAPSHOT_TOOLBAR, Tr::tr("Screenshot"), Tr::tr("Screenshot (Ctrl + Shift + C)"), "Ctrl+Shift+C", false},
        {Utils::Icon({{":/scxmleditor/images/icon-export-canvas.png",  Utils::Theme::IconsBaseColor}}), Tr::tr("Export to Image"), Tr::tr("Export to Image"), "Ctrl+Shift+E", false},
        {Utils::Icon({{":/utils/images/namespace.png", Utils::Theme::IconsBaseColor}}), Tr::tr("Toggle Full Namespace"), Tr::tr("Toggle Full Namespace"), "Ctrl+Shift+N", true},

        {Utils::Icon({{":/scxmleditor/images/align_left.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Align Left"), Tr::tr("Align Left (Ctrl+L,1)"), "Ctrl+L,1", false},
        {Utils::Icon({{":/scxmleditor/images/align_right.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Align Right"), Tr::tr("Align Right (Ctrl+L,2)"), "Ctrl+L,2", false},
        {Utils::Icon({{":/scxmleditor/images/align_top.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Align Top"), Tr::tr("Align Top (Ctrl+L,3)"), "Ctrl+L,3", false},
        {Utils::Icon({{":/scxmleditor/images/align_bottom.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Align Bottom"), Tr::tr("Align Bottom (Ctrl+L,4)"), "Ctrl+L,4", false},
        {Utils::Icon({{":/scxmleditor/images/align_horizontal.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Align Horizontal"), Tr::tr("Align Horizontal (Ctrl+L,5)"), "Ctrl+L,5", false},
        {Utils::Icon({{":/scxmleditor/images/align_vertical.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Align Vertical"), Tr::tr("Align Vertical (Ctrl+L,6)"), "Ctrl+L,6", false},
        {Utils::Icon({{":/scxmleditor/images/adjust_width.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Adjust Width"), Tr::tr("Adjust Width (Ctrl+L,7)"), "Ctrl+L,7", false},
        {Utils::Icon({{":/scxmleditor/images/adjust_height.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Adjust Height"), Tr::tr("Adjust Height (Ctrl+L,8)"), "Ctrl+L,8", false},
        {Utils::Icon({{":/scxmleditor/images/adjust_size.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), Tr::tr("Adjust Size"), Tr::tr("Adjust Size (Ctrl+L,9)"), "Ctrl+L,9", false},

        {Utils::Icon(":/scxmleditor/images/statistics.png"), Tr::tr("Show Statistics..."), Tr::tr("Show Statistics"), "", false}
    };

    // Init actions
    for (const auto &info: actionInfos) {
        auto action = new QAction(info.icon.icon(), info.name, this);
        action->setCheckable(info.checkable);
        action->setToolTip(info.tooltip);
        action->setShortcut(QKeySequence(QLatin1String(info.keyseq)));

        m_actions << action;
    }
}

QAction *ActionHandler::action(ActionType type) const
{
    if (type >= ActionZoomIn && type < ActionLast)
        return m_actions[type];

    return nullptr;
}
