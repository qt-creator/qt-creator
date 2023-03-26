// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1

Window {
    id: imageViewer
    minimumWidth: viewerImage.width
    minimumHeight: viewerImage.height
    function open(source) {
        viewerImage.source = source
        width = viewerImage.implicitWidth + 20
        height = viewerImage.implicitHeight + 20
        title = source
        visible = true
    }
    Image {
        id: viewerImage
        anchors.centerIn: parent
    }
}
