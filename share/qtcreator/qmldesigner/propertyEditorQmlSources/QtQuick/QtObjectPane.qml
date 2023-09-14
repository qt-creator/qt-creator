// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

PropertyEditorPane {
    id: itemPane

    ComponentSection {}

    Column {
        anchors.left: parent.left
        anchors.right: parent.right

        Loader {
            id: specificsTwo

            property string theSource: specificQmlData

            anchors.left: parent.left
            anchors.right: parent.right
            visible: specificsTwo.theSource !== ""
            sourceComponent: specificQmlComponent

            onTheSourceChanged: {
                specificsTwo.active = false
                specificsTwo.active = true
            }
        }

        Loader {
            id: specificsOne
            anchors.left: parent.left
            anchors.right: parent.right
            source: specificsUrl
        }
    }
}
