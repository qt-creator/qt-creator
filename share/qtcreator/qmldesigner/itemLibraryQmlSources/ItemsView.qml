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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.0
import "../common"

import QtQuick.Layouts 1.0
import HelperWidgets 2.0

import QtQuickDesignerTheme 1.0

/* The view displaying the item grid.

The following Qml context properties have to be set:
- listmodel itemLibraryModel
- int itemLibraryIconWidth
- int itemLibraryIconHeight

itemLibraryModel has to have the following structure:

ListModel {
ListElement {
int sectionLibId
string sectionName
list sectionEntries: [
ListElement {
int itemLibId
string itemName
pixmap itemPixmap
},
...
]
}
...
}
*/


ScrollView {
    id: itemsView

    Item {
        id: styleConstants
        readonly property color backgroundColor: Theme.qmlDesignerBackgroundColorDarkAlternate()
        readonly property color lighterBackgroundColor: Theme.color(Theme.FancyToolBarSeparatorColor)

        property int textWidth: 58
        property int textHeight: Theme.smallFontPixelSize() * 2

        property int cellHorizontalMargin: 1
        property int cellVerticalSpacing: 2
        property int cellVerticalMargin: 4

        // the following depend on the actual shape of the item delegate
        property int cellWidth: textWidth + 2 * cellHorizontalMargin
        property int cellHeight: itemLibraryIconHeight + textHeight +
        2 * cellVerticalMargin + cellVerticalSpacing
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: styleConstants.backgroundColor
    }

    style: DesignerScrollViewStyle {

    }

    Flickable {
        contentHeight: column.height
        Column {
            id: column
            Repeater {
                model: itemLibraryModel  // to be set in Qml context
                delegate: Section {
                    width: itemsView.viewport.width
                    caption: sectionName // to be set by model
                    visible: sectionVisible
                    topPadding: 2
                    leftPadding: 2
                    rightPadding: 1
                    expanded: sectionExpanded
                    onExpandedChanged: itemLibraryModel.setExpanded(expanded, sectionName);
                    Grid {
                        id: itemGrid

                        columns: parent.width / styleConstants.cellWidth
                        property int flexibleWidth: (parent.width - styleConstants.cellWidth * columns) / columns

                        Repeater {
                            model: sectionEntries
                            delegate: ItemDelegate {
                                visible: itemVisible
                                width: styleConstants.cellWidth + itemGrid.flexibleWidth
                                height: styleConstants.cellHeight
                            }
                        }
                    }
                }
            }
        }
    }
}
