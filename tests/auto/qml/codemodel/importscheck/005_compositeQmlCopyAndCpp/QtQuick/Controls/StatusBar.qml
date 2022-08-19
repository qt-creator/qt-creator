// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype StatusBar
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup applicationwindow
    \brief Contains status information in your app.

    The common way of using StatusBar is in relation to \l ApplicationWindow.

    Note that the StatusBar does not provide a layout of its own, but requires
    you to position its contents, for instance by creating a \l RowLayout.

    If only a single item is used within the StatusBar, it will resize to fit the implicitHeight
    of its contained item. This makes it particularly suitable for use together with layouts.
    Otherwise the height is platform dependent.

    \code
    import QtQuick.Controls 1.1
    import QtQuick.Layouts 1.0

    ApplicationWindow {
        statusBar: StatusBar {
            RowLayout {
                Label { text: "Read Only" }
            }
        }
    }
    \endcode
*/

Item {
    id: statusbar

    activeFocusOnTab: false
    Accessible.role: Accessible.StatusBar

    width: parent ? parent.width : implicitWidth
    implicitWidth: container.leftMargin + container.rightMargin + container.calcWidth()
    implicitHeight: Math.max(container.topMargin + container.bottomMargin + container.calcHeight(),
                             loader.item ? loader.item.implicitHeight : 19)

    /*! \internal */
    property Component style: Qt.createComponent(Settings.style + "/StatusBarStyle.qml", statusbar)

    /*! \internal */
    property alias __style: styleLoader.item

    /*! \internal */
    default property alias __content: container.data

    /*!
        \qmlproperty Item StatusBar::contentItem

        This property holds the content Item of the status bar.

        Items declared as children of a StatusBar are automatically parented to the StatusBar's contentItem.
        Items created dynamically need to be explicitly parented to the contentItem:

        \note The implicit size of the StatusBar is calculated based on the size of its content. If you want to anchor
        items inside the status bar, you must specify an explicit width and height on the StatusBar itself.
    */
    readonly property alias contentItem: container

    data: [
        Loader {
            id: loader
            anchors.fill: parent
            sourceComponent: styleLoader.item ? styleLoader.item.panel : null
            onLoaded: item.z = -1
            Loader {
                id: styleLoader
                property alias __control: statusbar
                sourceComponent: style
            }
        },
        Item {
            id: container
            z: 1
            focus: true
            anchors.fill: parent

            anchors.topMargin: topMargin
            anchors.leftMargin: leftMargin
            anchors.rightMargin: rightMargin
            anchors.bottomMargin: bottomMargin

            property int topMargin: __style ? __style.padding.top : 0
            property int bottomMargin: __style ? __style.padding.bottom : 0
            property int leftMargin: __style ? __style.padding.left : 0
            property int rightMargin: __style ? __style.padding.right : 0

            property Item layoutItem: container.children.length === 1 ? container.children[0] : null
            function calcWidth() { return (layoutItem ? (layoutItem.implicitWidth || layoutItem.width) +
                                                          (layoutItem.anchors.fill ? layoutItem.anchors.leftMargin +
                                                                                     layoutItem.anchors.rightMargin : 0) :
                                                          loader.item ? loader.item.implicitWidth: 0) }
            function calcHeight () { return (layoutItem ? (layoutItem.implicitHeight || layoutItem.height) +
                                                          (layoutItem.anchors.fill ? layoutItem.anchors.topMargin +
                                                                                     layoutItem.anchors.bottomMargin : 0) : loader.item ? loader.item.implicitHeight : 0) }
        }]
}
