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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Dialog {
    id: root

    title: qsTr("Bundle material might be in use")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 300
    modal: true

    property var targetBundleMaterial

    contentItem: Column {
        spacing: 20
        width: parent.width

        Text {
            id: folderNotEmpty

            text: qsTr("If the material you are removing is in use, it might cause the project to malfunction.\n\nAre you sure you want to remove the material?")
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            anchors.right: parent.right
            anchors.left: parent.left
            leftPadding: 10
            rightPadding: 10

            Keys.onEnterPressed: btnRemove.onClicked()
            Keys.onReturnPressed: btnRemove.onClicked()
        }

        Row {
            anchors.right: parent.right
            Button {
                id: btnRemove

                text: qsTr("Remove")

                onClicked: {
                    materialBrowserBundleModel.removeFromProject(root.targetBundleMaterial)
                    root.accept()
                }
            }

            Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    onOpened: folderNotEmpty.forceActiveFocus()
}
