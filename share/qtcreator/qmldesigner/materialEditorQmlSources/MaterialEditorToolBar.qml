/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme
import ToolBarAction 1.0

Rectangle {
    id: root

    color: StudioTheme.Values.themeSectionHeadBackground
    width: row.width
    height: 40

    signal toolBarAction(int action)

    Row {
        id: row

        anchors.verticalCenter: parent.verticalCenter
        leftPadding: 6

        IconButton {
            icon: StudioTheme.Constants.applyMaterialToSelected

            normalColor: "transparent"
            iconSize: StudioTheme.Values.bigIconFontSize
            buttonSize: root.height
            enabled: hasMaterial && hasQuick3DImport
            onClicked: root.toolBarAction(ToolBarAction.ApplyToSelected)
            tooltip: qsTr("Apply material to selected model.")
        }

        IconButton {
            icon: StudioTheme.Constants.newMaterial

            normalColor: "transparent"
            iconSize: StudioTheme.Values.bigIconFontSize
            buttonSize: root.height
            enabled: hasQuick3DImport
            onClicked: root.toolBarAction(ToolBarAction.AddNewMaterial)
            tooltip: qsTr("Add a new material.")
        }

        IconButton {
            icon: StudioTheme.Constants.deleteMaterial

            normalColor: "transparent"
            iconSize: StudioTheme.Values.bigIconFontSize
            buttonSize: root.height
            enabled: hasMaterial && hasQuick3DImport
            onClicked: root.toolBarAction(ToolBarAction.DeleteCurrentMaterial)
            tooltip: qsTr("Delete current material.")
        }

        IconButton {
            icon: StudioTheme.Constants.openMaterialBrowser

            normalColor: "transparent"
            iconSize: StudioTheme.Values.bigIconFontSize
            buttonSize: root.height
            enabled: hasMaterial && hasQuick3DImport
            onClicked: root.toolBarAction(ToolBarAction.OpenMaterialBrowser)
            tooltip: qsTr("Open material browser.")
        }
    }
}
