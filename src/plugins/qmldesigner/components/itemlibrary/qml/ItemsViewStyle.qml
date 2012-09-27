/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

import QtQuick 1.0

// the style used the items view

Item {
    property string backgroundColor: "#4f4f4f"
    property string raisedBackgroundColor: "#e0e0e0"

    property string scrollbarColor: "#444444"
    property string scrollbarBorderColor: "#333333"
    property string scrollbarGradientStartColor: "#393939"
    property string scrollbarGradientMiddleColor: "#656565"
    property string scrollbarGradientEndColor: "#888888"
    property int scrollbarClickScrollAmount: 40
    property int scrollbarWheelDeltaFactor: 4

    property string itemNameTextColor: "#FFFFFF"

    property string gridLineLighter: "#5f5f5f"
    property string gridLineDarker: "#3f3f3f"

    property string sectionArrowColor: "#ffffff"
    property string sectionTitleTextColor: "#ffffff"
    property string sectionTitleBackgroundColor: "#656565"

    property int sectionTitleHeight: 18
    property int sectionTitleSpacing: 2

    property int iconWidth: 32
    property int iconHeight: 32

    property int textWidth: 95
    property int textHeight: 15

    property int cellHorizontalMargin: 5
    property int cellVerticalSpacing: 7
    property int cellVerticalMargin: 10

    // the following depend on the actual shape of the item delegate
    property int cellWidth: textWidth + 2 * cellHorizontalMargin
    property int cellHeight: itemLibraryIconHeight + textHeight +
    2 * cellVerticalMargin + cellVerticalSpacing
}

