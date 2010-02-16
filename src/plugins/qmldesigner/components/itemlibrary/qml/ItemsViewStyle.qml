/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

import Qt 4.6

Item {
    property string backgroundColor: "#707070"
    property string raisedBackgroundColor: "#e0e0e0"

    property string scrollbarBackgroundColor: "#505050"
    property string scrollbarHandleColor: "#303030"

    property string itemNameTextColor: "#FFFFFF"

    property string sectionTitleTextColor: "#f0f0f0"
    property string sectionTitleBackgroundColor: "#909090"

//    property string gridLineLighter: "#787878"
//    property string gridLineDarker: "#656565"
    property string gridLineLighter: "#808080"
    property string gridLineDarker: "#606060"

    property int sectionTitleHeight: 20
    property int sectionTitleSpacing: 2

    property int selectionSectionOffset: sectionTitleHeight + sectionTitleSpacing

    property int iconWidth: 32
    property int iconHeight: 32

    property int textWidth: 80
    property int textHeight: 15

    property int cellSpacing: 7
    property int cellMargin: 10

    property int cellWidth: textWidth + 2*cellMargin
    property int cellHeight:  itemLibraryIconHeight + textHeight + 2*cellMargin + cellSpacing
}

