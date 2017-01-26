/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "actionhandler.h"
#include "mytypes.h"

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
        { Utils::Icons::ZOOMIN_TOOLBAR, tr("Zoom In"), tr("Zoom In (Ctrl + + / Ctrl + Wheel)"), "Ctrl++", false },
        { Utils::Icons::ZOOMOUT_TOOLBAR, tr("Zoom Out"), tr("Zoom Out (Ctrl + - / Ctrl + Wheel)"), "Ctrl+-", false },
        { Utils::Icons::FITTOVIEW_TOOLBAR, tr("Fit to View"), tr("Fit to View (F11)"), "F11", false },
        { Utils::Icons::PAN_TOOLBAR, tr("Panning"), tr("Panning (Shift)"), "Shift", true },

        { Utils::Icons::ZOOM_TOOLBAR, tr("Magnifier"), tr("Magnifier Tool (Alt)"), "Alt", true },
        { Utils::Icon(":/scxmleditor/images/navigator.png"), tr("Navigator"), tr("Navigator (Ctrl+E)"), "Ctrl+E", true },

        { Utils::Icons::COPY_TOOLBAR, tr("Copy"), tr("Copy (Ctrl + C)"), "Ctrl+C", false },
        { Utils::Icons::CUT_TOOLBAR, tr("Cut"), tr("Cut (Ctrl + X)"), "Ctrl+X", false },
        { Utils::Icons::PASTE_TOOLBAR, tr("Paste"), tr("Paste (Ctrl + V)"), "Ctrl+V", false },
        { Utils::Icons::SNAPSHOT_TOOLBAR, tr("Screenshot"), tr("Screenshot (Ctrl + Shift + C)"), "Ctrl+Shift+C", false },
        { Utils::Icon({{":/scxmleditor/images/icon-export-canvas.png",  Utils::Theme::IconsBaseColor}}), tr("Export to Image"), tr("Export to Image"), "Ctrl+Shift+E", false },
        { Utils::Icon({{":/utils/images/namespace.png", Utils::Theme::IconsBaseColor}}), tr("Toggle Full Namespace"), tr("Toggle Full Namespace"), "Ctrl+Shift+N", true },

        { Utils::Icon({{":/scxmleditor/images/align_left.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Align Left"), tr("Align Left (Ctrl+L,1)"), "Ctrl+L,1", false },
        { Utils::Icon({{":/scxmleditor/images/align_right.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Align Right"), tr("Align Right (Ctrl+L,2)"), "Ctrl+L,2", false },
        { Utils::Icon({{":/scxmleditor/images/align_top.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Align Top"), tr("Align Top (Ctrl+L,3)"), "Ctrl+L,3", false },
        { Utils::Icon({{":/scxmleditor/images/align_bottom.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Align Bottom"), tr("Align Bottom (Ctrl+L,4)"), "Ctrl+L,4", false },
        { Utils::Icon({{":/scxmleditor/images/align_horizontal.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Align Horizontal"), tr("Align Horizontal (Ctrl+L,5)"), "Ctrl+L,5", false },
        { Utils::Icon({{":/scxmleditor/images/align_vertical.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Align Vertical"), tr("Align Vertical (Ctrl+L,6)"), "Ctrl+L,6", false },
        { Utils::Icon({{":/scxmleditor/images/adjust_width.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Adjust Width"), tr("Adjust Width (Ctrl+L,7)"), "Ctrl+L,7", false },
        { Utils::Icon({{":/scxmleditor/images/adjust_height.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Adjust Height"), tr("Adjust Height (Ctrl+L,8)"), "Ctrl+L,8", false },
        { Utils::Icon({{":/scxmleditor/images/adjust_size.png", Utils::Theme::PanelTextColorMid}}, Utils::Icon::Tint), tr("Adjust Size"), tr("Adjust Size (Ctrl+L,9)"), "Ctrl+L,9", false },

        { Utils::Icon(":/scxmleditor/images/statistics.png"), tr("Show Statistics..."), tr("Show Statistics"), "", false }
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
