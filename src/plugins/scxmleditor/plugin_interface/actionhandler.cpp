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
    using AH = ActionHandler;

    const struct {
        const Utils::Icon icon;
        QString name;
        QString tooltip;
        const char *keyseq;
        bool checkable;
    } actionInfos[] = {
        { Utils::Icons::ZOOMIN_TOOLBAR, AH::tr("Zoom In"), AH::tr("Zoom In (Ctrl + + / Ctrl + Wheel)"), "Ctrl++", false },
        { Utils::Icons::ZOOMOUT_TOOLBAR, AH::tr("Zoom Out"), AH::tr("Zoom Out (Ctrl + - / Ctrl + Wheel)"), "Ctrl+-", false },
        { Utils::Icons::FITTOVIEW_TOOLBAR, AH::tr("Fit to View"), AH::tr("Fit to View (F11)"), "F11", false },
        { Utils::Icon(":/scxmleditor/images/icon-pan.png"), AH::tr("Panning"), AH::tr("Panning (Shift)"), "Shift", true },

        { Utils::Icons::ZOOM_TOOLBAR, AH::tr("Magnifier"), AH::tr("Magnifier Tool (Alt)"), "Alt", true },
        { Utils::Icon(":/scxmleditor/images/navigator.png"), AH::tr("Navigator"), AH::tr("Navigator (Ctrl+E)"), "Ctrl+E", true },

        { Utils::Icon({{":/utils/images/editcopy.png",  Utils::Theme::IconsBaseColor}}), AH::tr("Copy"), AH::tr("Copy (Ctrl + C)"), "Ctrl+C", false },
        { Utils::Icon({{":/utils/images/editcut.png",  Utils::Theme::IconsBaseColor}}), AH::tr("Cut"), AH::tr("Cut (Ctrl + X)"), "Ctrl+X", false },
        { Utils::Icon({{":/utils/images/editpaste.png",  Utils::Theme::IconsBaseColor}}), AH::tr("Paste"), AH::tr("Paste (Ctrl + V)"), "Ctrl+V", false },
        { Utils::Icons::SNAPSHOT_TOOLBAR, AH::tr("Screenshot"), AH::tr("Screenshot (Ctrl + Shift + C)"), "Ctrl+Shift+C", false },
        { Utils::Icon({{":/scxmleditor/images/icon-export-canvas.png",  Utils::Theme::IconsBaseColor}}), AH::tr("Export to Image"), AH::tr("Export to Image"), "Ctrl+Shift+E", false },
        { Utils::Icon(":/scxmleditor/images/fullnamespace.png"), AH::tr("Toggle Full Namespace"), AH::tr("Toggle Full Namespace"), "Ctrl+Shift+N", true },

        { Utils::Icon({{":/scxmleditor/images/align_left.png", Utils::Theme::IconsBaseColor}}), AH::tr("Align Left"), AH::tr("Align Left (Ctrl+L,1)"), "Ctrl+L,1", false },
        { Utils::Icon({{":/scxmleditor/images/align_right.png", Utils::Theme::IconsBaseColor}}), AH::tr("Align Right"), AH::tr("Align Right (Ctrl+L,2)"), "Ctrl+L,2", false },
        { Utils::Icon({{":/scxmleditor/images/align_top.png", Utils::Theme::IconsBaseColor}}), AH::tr("Align Top"), AH::tr("Align Top (Ctrl+L,3)"), "Ctrl+L,3", false },
        { Utils::Icon({{":/scxmleditor/images/align_bottom.png", Utils::Theme::IconsBaseColor}}), AH::tr("Align Bottom"), AH::tr("Align Bottom (Ctrl+L,4)"), "Ctrl+L,4", false },
        { Utils::Icon({{":/scxmleditor/images/align_horizontal.png", Utils::Theme::IconsBaseColor}}), AH::tr("Align Horizontal"), AH::tr("Align Horizontal (Ctrl+L,5)"), "Ctrl+L,5", false },
        { Utils::Icon({{":/scxmleditor/images/align_vertical.png", Utils::Theme::IconsBaseColor}}), AH::tr("Align Vertical"), AH::tr("Align Vertical (Ctrl+L,6)"), "Ctrl+L,6", false },
        { Utils::Icon({{":/scxmleditor/images/adjust_width.png", Utils::Theme::IconsBaseColor}}), AH::tr("Adjust Width"), AH::tr("Adjust Width (Ctrl+L,7)"), "Ctrl+L,7", false },
        { Utils::Icon({{":/scxmleditor/images/adjust_height.png", Utils::Theme::IconsBaseColor}}), AH::tr("Adjust Height"), AH::tr("Adjust Height (Ctrl+L,8)"), "Ctrl+L,8", false },
        { Utils::Icon({{":/scxmleditor/images/adjust_size.png", Utils::Theme::IconsBaseColor}}), AH::tr("Adjust Size"), AH::tr("Adjust Size (Ctrl+L,9)"), "Ctrl+L,9", false },

        { Utils::Icon(":/scxmleditor/images/statistics.png"), AH::tr("Show Statistics..."), AH::tr("Show Statistics"), "", false }
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
