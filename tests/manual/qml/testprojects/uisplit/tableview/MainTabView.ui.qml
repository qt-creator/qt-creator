// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2
import "tabs"

TabView {
    id:frame
    focus:true

    width: 540
    height: 360

    property bool genteratedTabFrameVisible: false
    property bool genteratedTabHeaderVisible: false
    property bool genteratedTabSortIndicatorVisible: false
    property bool genteratedTabAlternatingRowColors: false

    Tab {
        title: "XmlListModel"

        TabXmlListModel {
            anchors.fill: parent
            anchors.margins: 12

            frameVisible: genteratedTabFrameVisible
            headerVisible: genteratedTabHeaderVisible
            sortIndicatorVisible: genteratedTabSortIndicatorVisible
            alternatingRowColors: genteratedTabAlternatingRowColors
        }
    }

    Tab {
        title: "Multivalue"

        TabMultivalue {
            anchors.fill: parent
            anchors.margins: 12

            frameVisible: genteratedTabFrameVisible
            headerVisible: genteratedTabHeaderVisible
            sortIndicatorVisible: genteratedTabSortIndicatorVisible
            alternatingRowColors: genteratedTabAlternatingRowColors
        }
    }

    Tab {
        title: "Generated"
        id: generatedTab

        TabGenerated {
            anchors.margins: 12
            anchors.fill: parent

            frameVisible: genteratedTabFrameVisible
            headerVisible: genteratedTabHeaderVisible
            sortIndicatorVisible: genteratedTabSortIndicatorVisible
            alternatingRowColors: genteratedTabAlternatingRowColors
        }
    }

    Tab {
        title: "Delegates"

        TabDelegates {
        }
    }
}
