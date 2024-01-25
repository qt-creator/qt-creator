// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

Item {
    id: root

    implicitWidth: 300
    implicitHeight: collectionListView.height

    property bool hasSelectedTarget

    ListView {
        id: collectionListView

        width: parent.width
        implicitHeight: contentHeight
        leftMargin: 0

        model: internalModels
        clip: true

        delegate: CollectionItem {
            width: collectionListView.width
            sourceType: collectionListView.model.sourceType
            hasSelectedTarget: root.hasSelectedTarget
            onDeleteItem: collectionListView.model.removeRow(index)
        }
    }
}
