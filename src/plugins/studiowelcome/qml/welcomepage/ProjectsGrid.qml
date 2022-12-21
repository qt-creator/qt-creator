// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9
import welcome 1.0

GridView {
    id: root
    cellHeight: 180
    cellWidth: 285

    clip: true

    signal itemSelected(int index, variant item)

    delegate: HoverOverDesaturate {
        id: hoverOverDesaturate
        imageSource: typeof(thumbnail) === "undefined" ? previewUrl : thumbnail
        labelText: displayName
        downloadIcon: typeof(showDownload) === "undefined" ? false : showDownload;
        onClicked: root.itemSelected(index, root.model.get(index))

        SequentialAnimation {
            id: animation
            running: hoverOverDesaturate.visible

            PropertyAction {
                target: hoverOverDesaturate
                property: "scale"
                value: 0.0
            }
            PauseAnimation {
                duration: model.index > 0 ? 100 * model.index : 0
            }
            NumberAnimation {
                target: hoverOverDesaturate
                property: "scale"
                from: 0.0
                to: 1.0
                duration: 200
                easing.type: Easing.InOutExpo
            }
        }
    }
}
