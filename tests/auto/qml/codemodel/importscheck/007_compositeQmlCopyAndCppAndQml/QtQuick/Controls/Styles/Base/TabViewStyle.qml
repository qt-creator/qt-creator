/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype TabViewStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.1
    \ingroup viewsstyling
    \brief Provides custom styling for TabView

\qml
    TabView {
        id: frame
        anchors.fill: parent
        anchors.margins: 4
        Tab { title: "Tab 1" }
        Tab { title: "Tab 2" }
        Tab { title: "Tab 3" }

        style: TabViewStyle {
            frameOverlap: 1
            tab: Rectangle {
                color: styleData.selected ? "steelblue" :"lightsteelblue"
                border.color:  "steelblue"
                implicitWidth: Math.max(text.width + 4, 80)
                implicitHeight: 20
                radius: 2
                Text {
                    id: text
                    anchors.centerIn: parent
                    text: styleData.title
                    color: styleData.selected ? "white" : "black"
                }
            }
            frame: Rectangle { color: "steelblue" }
        }
    }
\endqml

*/

Style {

    /*! The \l ScrollView attached to this style. */
    readonly property TabView control: __control

    /*! This property holds whether the user can move the tabs.
        Tabs are not movable by default. */
    property bool tabsMovable: false

    /*! This property holds the horizontal alignment of
        the tab buttons. Supported values are:
        \list
        \li Qt.AlignLeft (default)
        \li Qt.AlignHCenter
        \li Qt.AlignRight
        \endlist
    */
    property int tabsAlignment: Qt.AlignLeft

    /*! This property holds the amount of overlap there are between
      individual tab buttons. */
    property int tabOverlap: 1

    /*! This property holds the amount of overlap there are between
      individual tab buttons and the frame. */
    property int frameOverlap: 2

    /*! This defines the tab frame. */
    property Component frame: Rectangle {
        color: "#dcdcdc"
        border.color: "#aaa"

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "#66ffffff"
            anchors.margins: 1
        }
    }

    /*! This defines the tab. You can access the tab state through the
        \c styleData property, with the following properties:

        \table
            \row \li readonly property int \b styleData.index \li This is the current tab index.
            \row \li readonly property bool \b styleData.selected \li This is the active tab.
            \row \li readonly property string \b styleData.title \li Tab title text.
            \row \li readonly property bool \b styleData.nextSelected \li The next tab is selected.
            \row \li readonly property bool \b styleData.previousSelected \li The previous tab is selected.
            \row \li readonly property bool \b styleData.hovered \li The tab is being hovered.
            \row \li readonly property bool \b styleData.activeFocus \li The tab button has keyboard focus.
            \row \li readonly property bool \b styleData.availableWidth \li The available width for the tabs.
        \endtable
    */
    property Component tab: Item {
        scale: control.tabPosition === Qt.TopEdge ? 1 : -1

        property int totalOverlap: tabOverlap * (control.count - 1)
        property real maxTabWidth: (styleData.availableWidth + totalOverlap) / control.count

        implicitWidth: Math.round(Math.min(maxTabWidth, textitem.implicitWidth + 20))
        implicitHeight: Math.round(textitem.implicitHeight + 10)

        clip: true
        Item {
            anchors.fill: parent
            anchors.bottomMargin: styleData.selected ? 0 : 2
            clip: true
            BorderImage {
                anchors.fill: parent
                source: styleData.selected ? "images/tab_selected.png" : "images/tab.png"
                border.top: 6
                border.bottom: 6
                border.left: 6
                border.right: 6
                anchors.topMargin: styleData.selected ? 0 : 1
            }
        }
        Rectangle {
            anchors.fill: textitem
            anchors.margins: -1
            anchors.leftMargin: -3
            anchors.rightMargin: -3
            visible: (styleData.activeFocus && styleData.selected)
            height: 6
            radius: 3
            color: "#224f9fef"
            border.color: "#47b"
        }
        Text {
            id: textitem
            anchors.centerIn: parent
            anchors.alignWhenCentered: true
            text: styleData.title
            renderType: Text.NativeRendering
            scale: control.tabPosition === Qt.TopEdge ? 1 : -1
            color: __syspal.text
        }
    }

    /*! This defines the left corner. */
    property Component leftCorner: null

    /*! This defines the right corner. */
    property Component rightCorner: null

    /*! This defines the tab bar background. */
    property Component tabBar: null
}
