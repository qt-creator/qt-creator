// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

PropertyEditorPane {
    id: itemPane

    ComponentSection {
        showState: majorQtQuickVersion >= 6
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right

        DynamicPropertiesSection {
            propertiesModel: SelectionDynamicPropertiesModel {}
            visible: !hasMultiSelection
        }

        Loader {
            id: specificsTwo
            anchors.left: parent.left
            anchors.right: parent.right
            visible: theSource !== ""
            sourceComponent: specificQmlComponent

            property string theSource: specificQmlData

            onTheSourceChanged: {
                active = false
                active = true
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
