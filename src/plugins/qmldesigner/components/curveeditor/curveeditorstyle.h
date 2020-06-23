/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#pragma once

#include "detail/shortcut.h"

#include <utils/hostosinfo.h>

#include <QBitmap>
#include <QBrush>
#include <QColor>
#include <QDialog>
#include <QIcon>
#include <QKeySequence>

namespace DesignTools {

struct TreeItemStyleOption
{
    double margins;
    QIcon pinnedIcon = QIcon(":/curveeditor/images/treeview_pin.png");
    QIcon unpinnedIcon = QIcon(":/curveeditor/images/treeview_unpin.png");
    QIcon lockedIcon = QIcon(":/curveeditor/images/treeview_lock.png");
    QIcon unlockedIcon = QIcon(":/curveeditor/images/treeview_unlock.png");
};

struct HandleItemStyleOption
{
    double size = 10.0;
    double lineWidth = 1.0;
    QColor color = QColor(200, 0, 0);
    QColor selectionColor = QColor(200, 200, 200);
    QColor activeColor = QColor(0, 200, 0);
    QColor hoverColor = QColor(200, 0, 200);
};

struct KeyframeItemStyleOption
{
    double size = 10.0;
    QColor color = QColor(200, 200, 0);
    QColor selectionColor = QColor(200, 200, 200);
    QColor lockedColor = QColor(50, 50, 50);
    QColor unifiedColor = QColor(250, 50, 250);
    QColor splitColor = QColor(0, 250, 0);
};

struct CurveItemStyleOption
{
    double width = 1.0;
    QColor color = QColor(0, 200, 0);
    QColor selectionColor = QColor(200, 200, 200);
    QColor easingCurveColor = QColor(200, 0, 200);
    QColor lockedColor = QColor(50, 50, 50);
    QColor hoverColor = QColor(200, 0, 200);
};

struct PlayheadStyleOption
{
    double width = 20.0;
    double radius = 4.0;
    QColor color = QColor(200, 200, 0);
};

struct Shortcuts
{
    Shortcut newSelection = Shortcut(Qt::LeftButton);
    Shortcut addToSelection = Shortcut(Qt::LeftButton, Qt::ControlModifier | Qt::ShiftModifier);
    Shortcut removeFromSelection = Shortcut(Qt::LeftButton, Qt::ShiftModifier);
    Shortcut toggleSelection = Shortcut(Qt::LeftButton, Qt::ControlModifier);

    Shortcut zoom = Shortcut(Qt::RightButton, Qt::AltModifier);
    Shortcut pan = Shortcut(Qt::MiddleButton, Qt::AltModifier);
    Shortcut frameAll = Shortcut(Qt::NoModifier, Qt::Key_A);

    Shortcut insertKeyframe = Shortcut(Qt::MiddleButton, Qt::NoModifier);

    Shortcut deleteKeyframe = Utils::HostOsInfo::isMacHost()
                                  ? Shortcut(Qt::NoModifier, Qt::Key_Backspace)
                                  : Shortcut(Qt::NoModifier, Qt::Key_Delete);
};

struct CurveEditorStyle
{
    Shortcuts shortcuts;

    QBrush backgroundBrush = QBrush(QColor(5, 0, 100));
    QBrush backgroundAlternateBrush = QBrush(QColor(0, 0, 50));
    QColor fontColor = QColor(200, 200, 200);
    QColor iconColor = QColor(128, 128, 128);
    QColor iconHoverColor = QColor(170, 170, 170);
    QColor gridColor = QColor(128, 128, 128);
    double canvasMargin = 5.0;
    int zoomInWidth = 100;
    int zoomInHeight = 100;
    double timeAxisHeight = 40.0;
    double timeOffsetLeft = 10.0;
    double timeOffsetRight = 10.0;
    QColor rangeBarColor = QColor(128, 128, 128);
    QColor rangeBarCapsColor = QColor(50, 50, 255);
    double valueAxisWidth = 60.0;
    double valueOffsetTop = 10.0;
    double valueOffsetBottom = 10.0;
    double labelDensityY = 1.5;

    HandleItemStyleOption handleStyle;
    KeyframeItemStyleOption keyframeStyle;
    CurveItemStyleOption curveStyle;
    TreeItemStyleOption treeItemStyle;
    PlayheadStyleOption playhead;
};

inline QPixmap pixmapFromIcon(const QIcon &icon, const QSize &size, const QColor &color)
{
    QPixmap pixmap = icon.pixmap(size);
    QPixmap mask(pixmap.size());
    mask.fill(color);
    mask.setMask(pixmap.createMaskFromColor(Qt::transparent));
    return mask;
}

} // End namespace DesignTools.
