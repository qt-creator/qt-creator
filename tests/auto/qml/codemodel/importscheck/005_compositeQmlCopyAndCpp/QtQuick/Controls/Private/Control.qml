// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls.Styles 1.1

/*!
        \qmltype Control
        \internal
        \qmlabstract
        \inqmlmodule QtQuick.Controls.Private
*/
FocusScope {
    id: root

    /*! \qmlproperty Component Control::style

        The style Component for this control.
        \sa {Qt Quick Controls Styles QML Types}

    */
    property Component style

    /*! \internal */
    property QtObject __style: styleLoader.item

    /*! \internal */
    property Item __panel: panelLoader.item

    /*! \internal */
    property var styleHints

    implicitWidth: __panel ? __panel.implicitWidth: 0
    implicitHeight: __panel ? __panel.implicitHeight: 0
    baselineOffset: __panel ? __panel.baselineOffset: 0
    activeFocusOnTab: false

    /*! \internal */
    property alias __styleData: styleLoader.styleData

    Loader {
        id: panelLoader
        anchors.fill: parent
        sourceComponent: __style ? __style.panel : null
        onStatusChanged: if (status === Loader.Error) console.error("Failed to load Style for", root)
        Loader {
            id: styleLoader
            sourceComponent: style
            property Item __control: root
            property QtObject styleData: null
            onStatusChanged: {
                if (status === Loader.Error)
                    console.error("Failed to load Style for", root)
            }
        }
    }
}
