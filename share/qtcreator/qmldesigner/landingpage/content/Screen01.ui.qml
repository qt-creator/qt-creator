/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick Studio Components.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
import QtQuick.Controls 6.2
import QtQuick.Layouts 1.15
import LandingPageApi
import LandingPage as Theme

Rectangle {
    id: root

    width: 1024
    height: 768
    color: Theme.Colors.backgroundPrimary

    Connections {
        target: openQds
        function onClicked() { LandingPageApi.openQds(rememberCheckbox.checkState === Qt.Checked) }
    }

    Connections {
        target: openQtc
        function onClicked() { LandingPageApi.openQtc(rememberCheckbox.checkState === Qt.Checked) }
    }

    states: [
        State {
            name: "large"
            when: root.width > Theme.Values.layoutBreakpointLG
            PropertyChanges {
                target: Theme.Values
                fontSizeTitle: Theme.Values.fontSizeTitleLG
                fontSizeSubtitle: Theme.Values.fontSizeSubtitleLG
            }
            PropertyChanges {
                target: buttonBoxGrid
                columns: 2
            }
        },
        State {
            name: "medium"
            when: root.width <= Theme.Values.layoutBreakpointLG
                  && root.width > Theme.Values.layoutBreakpointMD
            PropertyChanges {
                target: Theme.Values
                fontSizeTitle: Theme.Values.fontSizeTitleMD
                fontSizeSubtitle: Theme.Values.fontSizeSubtitleMD
            }
            PropertyChanges {
                target: buttonBoxGrid
                columns: 2
            }
        },
        State {
            name: "small"
            when: root.width <= Theme.Values.layoutBreakpointMD
            PropertyChanges {
                target: Theme.Values
                fontSizeTitle: Theme.Values.fontSizeTitleSM
                fontSizeSubtitle: Theme.Values.fontSizeSubtitleSM
            }
            PropertyChanges {
                target: buttonBoxGrid
                columns: 1
            }
        }
    ]

    ScrollView {
        id: scrollView
        anchors.fill: root

        Column {
            id: layout
            spacing: 0
            width: scrollView.width

            Item {
                width: layout.width
                height: logoSection.childrenRect.height + (2 * Theme.Values.spacing)

                Column {
                    id: logoSection
                    spacing: 10
                    anchors.centerIn: parent

                    Image {
                        id: qdsLogo
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: "logo.png"
                    }

                    Text {
                        id: qdsText
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Qt Design Studio")
                        font.pixelSize: Theme.Values.fontSizeTitle
                        font.family: Theme.Values.baseFont
                        color: Theme.Colors.text
                    }
                }
            }

            InstallQdsStatusBlock {
                id: installQdsStatusBlock
                width: parent.width
                visible: !LandingPageApi.qdsInstalled
            }

            ProjectInfoStatusBlock {
                id: projectInfoStatusBlock
                width: parent.width
                visible: !installQdsStatusBlock.visible
                projectFileExists: LandingPageApi.projectFileExists
                qtVersion: LandingPageApi.qtVersion
                qdsVersion: LandingPageApi.qdsVersion
            }

            GridLayout {
                id: buttonBoxGrid
                anchors.horizontalCenter: parent.horizontalCenter
                columns: 2
                rows: 2
                columnSpacing: 3 * Theme.Values.spacing
                rowSpacing: Theme.Values.spacing

                property int tmpWidth: textMetrics.width

                TextMetrics {
                    id: textMetrics
                    text: openQtcText.text.length > openQdsText.text.length ? openQtcText.text
                                                                            : openQdsText.text
                    font.pixelSize: Theme.Values.fontSizeSubtitle
                    font.family: Theme.Values.baseFont
                }

                Column {
                    id: openQdsBox
                    Layout.alignment: Qt.AlignHCenter

                    PageText {
                        id: openQdsText
                        width: buttonBoxGrid.tmpWidth
                        padding: Theme.Values.spacing
                        text: qsTr("Open with Qt Design Studio")
                        wrapMode: Text.NoWrap
                    }

                    PushButton {
                        id: openQds
                        text: qsTr("Open")
                        enabled: LandingPageApi.qdsInstalled
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                Column {
                    id: openQtcBox
                    Layout.alignment: Qt.AlignHCenter

                    PageText {
                        id: openQtcText
                        width: buttonBoxGrid.tmpWidth
                        padding: Theme.Values.spacing
                        text: qsTr("Open with Qt Creator - Text Mode")
                        wrapMode: Text.NoWrap
                    }

                    PushButton {
                        id: openQtc
                        text: qsTr("Open")
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                CustomCheckBox {
                    id: rememberCheckbox
                    text: qsTr("Remember my choice")
                    font.family: Theme.Values.baseFont
                    Layout.columnSpan: buttonBoxGrid.columns
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: Theme.Values.spacing
                    Layout.bottomMargin: Theme.Values.spacing
                }
            }
        }
    }
}
