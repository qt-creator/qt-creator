/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import StudioFonts 1.0
import projectmodel 1.0
import usagestatistics 1.0
import QtQuick.Shapes 1.0

Rectangle {
    id: welcome_splash
    anchors.fill: parent
    clip: true

    gradient: Gradient {
        orientation: Gradient.Horizontal

        GradientStop {
            position: 0.0
            color: "#1d212a"
        }
        GradientStop {
            position: 1.0
            color: "#232c56"
        }
    }

    signal goNext
    signal closeClicked
    signal configureClicked

    property bool doNotShowAgain: true
    property bool loadingPlugins: true

    width: 600
    height: 720
    visible: true
    color: "#1d212a"

    Rectangle {
        id: qtGreen
        color: "#515151"
        anchors.fill: parent
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
    }

    RectangleItem {
        id: background
        opacity: 0.825
        anchors.fill: parent
        gradient: LinearGradient {
            x1: 0
            stops: [
                GradientStop {
                    position: 0
                    color: "#29323c"
                },
                GradientStop {
                    position: 1
                    color: "#485563"
                }
            ]
            x2: 640
            y2: 800
            y1: 0
        }
        topRightRadius: 0
        anchors.topMargin: 0
        bottomLeftRadius: 0
        borderMode: 1
        fillColor: "#2d2d2d"
        bottomRightRadius: 0
        topLeftRadius: 0
        topRightBevel: true
        bottomLeftBevel: true
        strokeColor: "#00ff0000"
    }

    RectangleItem {
        id: topBar
        opacity: 0.239
        anchors.fill: parent
        anchors.bottomMargin: 550
        strokeColor: "#00ff0000"
        bottomRightRadius: 0
        topRightBevel: true
        fillColor: "#2d2d2d"
        bottomLeftRadius: 0
        topRightRadius: 0
        borderMode: 1
        anchors.topMargin: 0
        topLeftRadius: 0
        bottomLeftBevel: true
    }

    RectangleItem {
        id: bottomBar
        opacity: 0.534
        visible: true
        anchors.fill: parent
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        strokeColor: "#00ff0000"
        anchors.bottomMargin: 0
        bottomRightRadius: 0
        bottomLeftRadius: 0
        fillColor: "#2d2d2d"
        topRightBevel: true
        topRightRadius: 0
        borderMode: 1
        anchors.topMargin: 539
        topLeftRadius: 0
        bottomLeftBevel: true
    }

    MyEllipse {
        id: ellipse
        x: 0
        y: 204
        width: 640
        height: 391
        opacity: 0.05
    }

    Image {
        id: logo
        anchors.top: parent.top
        anchors.margins: 10
        smooth: true
        source: "welcome_windows/logo.png"
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Text {
        id: license_variant_text
        anchors.top: designStudioVersion.bottom
        color: "#ffffff"

        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 20
        anchors.horizontalCenter: parent.horizontalCenter

        text: {
            if (projectModel.communityVersion)
                return qsTr("Community Edition")
            if (projectModel.enterpriseVersion)
                return qsTr("Enterprise Edition")
            return qsTr("Professional Edition")
        }

        ProjectModel {
            id: projectModel
        }

        UsageStatisticModel {
            id: usageStatisticModel
        }
    }

    //DOF seems to do nothing, we should probably just remove it.
    Splash_Image25d {
        id: animated_artwork
        width: 628
        height: 377
        anchors.top: license_variant_text.bottom
        anchors.horizontalCenterOffset: 0
        scale: 1.1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 50
        clip: true
    }

    Text {
        id: help_us_text
        anchors.left: welcome_splash.left
        anchors.right: parent.right
        anchors.leftMargin: 20
        anchors.top: bottomBar.top
        color: "#FFFFFF"
        text: qsTr("Before we let you move on to your wonderful designs, help us make Qt Design Studio even betterby letting us know how you're using it. To do this, we would like to turn on automatic collection of pseudonymized Analytics and Crash Report Data.")
        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        anchors.topMargin: 25
        anchors.rightMargin: 20
    }

    RowLayout {
        anchors.right: parent.right
        anchors.bottom: welcome_splash.bottom
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        spacing: 20

        PushButton {
            text: qsTr("Turn Off")
            fontpixelSize: 14
            onClicked: {
                usageStatisticModel.setTelemetryEnabled(false)
                usageStatisticModel.setCrashReporterEnabled(false)
                welcome_splash.closeClicked()
            }
        }

        PushButton {
            text: qsTr("Turn On")
            forceHover: false
            fontpixelSize: 14
            onClicked: {
                usageStatisticModel.setTelemetryEnabled(true)
                usageStatisticModel.setCrashReporterEnabled(true)
                welcome_splash.closeClicked()
            }
        }
    }

    PushButton {
        y: 430
        text: qsTr("Learn More")
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        fontpixelSize: 14
        anchors.bottomMargin: 20
        anchors.leftMargin: 20
        onClicked: Qt.openUrlExternally(
                       "https://www.qt.io/terms-conditions/telemetry-privacy")
    }

    Row {
        y: 690
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 20
        layoutDirection: Qt.LeftToRight

        CheckBox {
            visible: true
            id: usageStatisticCheckBox
            text: qsTr("Send Usage Statistics")
            checked: usageStatisticModel.usageStatisticEnabled
            padding: 0
            spacing: 12

            onCheckedChanged: usageStatisticModel.setTelemetryEnabled(
                                  usageStatisticCheckBox.checked)

            contentItem: Text {
                text: usageStatisticCheckBox.text
                color: "#ffffff"
                leftPadding: usageStatisticCheckBox.indicator.width + usageStatisticCheckBox.spacing
                font.pixelSize: 12
            }
        }

        CheckBox {
            visible: true
            id: crashReportCheckBox
            text: qsTr("Send Crash Reports")
            spacing: 12
            checked: usageStatisticModel.crashReporterEnabled

            onCheckedChanged: {
                usageStatisticModel.setCrashReporterEnabled(
                            crashReportCheckBox.checked)
                welcome_splash.onPluginInitialized(true,
                                                   crashReportCheckBox.checked)
            }

            contentItem: Text {
                color: "#ffffff"
                text: crashReportCheckBox.text
                leftPadding: crashReportCheckBox.indicator.width + crashReportCheckBox.spacing
                font.pixelSize: 12
            }
            padding: 0
        }
    }

    Row {
        id: designStudioVersion
        anchors.top: logo.bottom
        anchors.topMargin: 5
        spacing: 10
        anchors.horizontalCenter: parent.horizontalCenter

        Text {
            id: qt_design_studio_text
            height: 45
            color: "#ffffff"
            text: qsTr("Qt Design Studio")
            font.pixelSize: 36
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: StudioFonts.titilliumWeb_light
        }

        Text {
            id: qt_design_studio_version_text
            height: 45
            color: "#fbfbfb"
            text: usageStatisticModel.version
            font.family: StudioFonts.titilliumWeb_light
            font.pixelSize: 36
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:1.33}D{i:22}
}
##^##*/
