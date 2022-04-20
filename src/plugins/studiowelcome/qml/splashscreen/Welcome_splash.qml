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

Rectangle {
    id: welcome_splash
    anchors.fill: parent

    gradient: Gradient {
        orientation: Gradient.Horizontal

        GradientStop { position: 0.0; color: "#1d212a" }
        GradientStop { position: 1.0; color: "#232c56" }
    }

    signal goNext
    signal closeClicked
    signal configureClicked

    property bool doNotShowAgain: true
    property bool loadingPlugins: true
    color: "#1d212a"

    Image {
        id: logo
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        width: 66 * 2
        height: 50 * 2
        smooth: true
        source: "welcome_windows/logo.png"
    }


    Text {
        id: qt_design_studio_text
        anchors.top: logo.top
        anchors.left: logo.right
        anchors.leftMargin: 10
        color: "#25709a"
        text: qsTr("Qt Design Studio")
        font.pixelSize: 36
        font.family: StudioFonts.titilliumWeb_light
    }

    Text {
        id: qt_design_studio_version_text
        anchors.left: qt_design_studio_text.right
        anchors.baseline: qt_design_studio_text.baseline
        anchors.leftMargin: 10
        color: "#25709a"
        text: usageStatisticModel.version

        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 36
    }

    Text {
        id: license_variant_text
        anchors.left: qt_design_studio_text.left
        anchors.top: qt_design_studio_text.bottom
        anchors.leftMargin: 5
        color: "#ffffff"

        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 20

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

    Dof_Effect {
        id: dof_effect
        anchors.top: qt_design_studio_text.bottom
        anchors.horizontalCenter: welcome_splash.horizontalCenter
        width: 442
        height: 480
        maskBlurSamples: 64
        maskBlurRadius: 32

        Splash_Image25d {
            id: animated_artwork
            width: dof_effect.width
            height: dof_effect.height
            clip: true
        }
    }

    Text {
        id: help_us_text
        anchors.left: welcome_splash.left
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.top: dof_effect.bottom
        anchors.topMargin: 10
        color: "#FFFFFF"
        text: qsTr("Before we let you move on to your wonderful designs, help us make Qt Design Studio even better by letting us know how you're using it.")

        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 18
        wrapMode: Text.WordWrap
        anchors.rightMargin: 10
    }

    ColumnLayout {
        id: columnLayout
        anchors.left: parent.left
        anchors.top: help_us_text.bottom
        anchors.leftMargin: 10
        anchors.topMargin: 20
        spacing: 3

        CheckBox {
            visible: false
            id: usageStatisticCheckBox
            text: qsTr("Send Usage Statistics")
            checked: usageStatisticModel.usageStatisticEnabled
            padding: 0
            spacing: 12

            onCheckedChanged: usageStatisticModel.setTelemetryEnabled(usageStatisticCheckBox.checked)

            contentItem: Text {
                text: usageStatisticCheckBox.text
                color: "#ffffff"
                leftPadding: usageStatisticCheckBox.indicator.width + usageStatisticCheckBox.spacing
                font.pixelSize: 12
            }
        }

        CheckBox {
            visible: false
            id: crashReportCheckBox
            text: qsTr("Send Crash Reports")
            spacing: 12
            checked: usageStatisticModel.crashReporterEnabled

            onCheckedChanged: {
                usageStatisticModel.setCrashReporterEnabled(crashReportCheckBox.checked)
                welcome_splash.onPluginInitialized(true, crashReportCheckBox.checked)
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

    RowLayout {
        anchors.right: parent.right
        anchors.bottom: welcome_splash.bottom
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
        spacing: 20

        CustomButton {
            text: qsTr("Don't send")
            onClicked: {
                usageStatisticModel.setTelemetryEnabled(false)
                usageStatisticModel.setCrashReporterEnabled(false)
                welcome_splash.closeClicked()
            }
        }

        CustomButton {
            text: qsTr("Send analytics data")
            onClicked: {
                usageStatisticModel.setTelemetryEnabled(true)
                usageStatisticModel.setCrashReporterEnabled(true)
                welcome_splash.closeClicked()
            }
        }
    }

    CustomButton {
        y: 430
        text: qsTr("Learn More")
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.leftMargin: 10
        onClicked: Qt.openUrlExternally("https://www.qt.io/terms-conditions/telemetry-privacy")
    }
}
