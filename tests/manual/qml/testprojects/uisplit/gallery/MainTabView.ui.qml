// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.0
import QtQuick.Dialogs 1.0
import "content"

TabView {

    id: tabView

    tabPosition: controlPage.item ? controlPage.item.tabPosition : Qt.TopEdge

    width: 640
    height: 420

    Tab {
        id: controlPage
        title: "Controls"
        Controls {
            anchors.fill: parent
            enabled: tabView.enabled
        }
    }
    Tab {
        title: "Itemviews"
        ModelView {
            anchors.fill: parent
            anchors.margins: Qt.platform.os === "osx" ? 12 : 6
        }
    }
    Tab {
        title: "Styles"
        Styles {
            anchors.fill: parent
        }
    }
    Tab {
        title: "Layouts"
        Layouts {
            anchors.fill:parent
            anchors.margins: 8
        }
    }
}
