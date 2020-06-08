/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick 2.7
import QtQuick.Controls 2.3
import StudioFonts 1.0
import QtQuick.Layouts 1.0
import projectmodel 1.0

Image {
    id: welcome_splash
    width: 800
    height: 480
    source: "welcome_windows/welcome_1.png"

    signal goNext
    signal closeClicked

    property alias doNotShowAgain: do_not_show_checkBox.checked
    property bool loadingPlugins: true

    Image {
        id: logo
        x: 14
        y: 8
        width: 76
        height: 66
        fillMode: Image.PreserveAspectFit
        source: "welcome_windows/logo.png"
    }

    Text {
        id: qt_design_studio
        x: 13
        y: 93
        width: 250
        height: 55
        color: "#4cd265"
        text: qsTr("Qt Design Studio")
        font.pixelSize: 36
        font.family: StudioFonts.titilliumWeb_light
    }

    Text {
        id: software_for_ui
        x: 15
        y: 141
        width: 250
        height: 30
        color: "#ffffff"
        text: qsTr("Software for UI and UX Designers")
        renderType: Text.QtRendering
        font.pixelSize: 18
        font.family: StudioFonts.titilliumWeb_light
    }

    Text {
        id: copyright
        x: 15
        y: 183
        width: 270
        height: 24
        color: "#ffffff"
        text: qsTr("Copyright 2008 - 2020 The Qt Company")
        font.pixelSize: 16
        font.family: StudioFonts.titilliumWeb_light
    }

    Text {
        id: all_rights_reserved
        x: 15
        y: 207
        width: 250
        height: 24
        color: "#ffffff"
        text: qsTr("All Rights Reserved")
        font.pixelSize: 16
        font.family: StudioFonts.titilliumWeb_light
    }

    Text {
        id: marketing_1
        x: 16
        y: 302
        width: 355
        height: 31
        color: "#ffffff"
        text: qsTr("Multi-paradigm language for creating highly dynamic applications.")
        wrapMode: Text.WordWrap
        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 12
        font.wordSpacing: 0
    }

    Text {
        id: marketing_2
        x: 16
        y: 323
        width: 311
        height: 31
        color: "#ffffff"
        text: qsTr("Run your concepts and prototypes on your final hardware.")
        wrapMode: Text.WordWrap
        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 12
        font.wordSpacing: 0
    }

    Text {
        id: marketing_3
        x: 16
        y: 344
        width: 311
        height: 31
        color: "#ffffff"
        text: qsTr("Seamless integration between designer and developer.")
        wrapMode: Text.WordWrap
        font.family: StudioFonts.titilliumWeb_light
        font.pixelSize: 12
        font.wordSpacing: 0
    }

    Dof_Effect {
        id: dof_Effect
        x: 358
        width: 442
        height: 480
        visible: true
        maskBlurSamples: 64
        maskBlurRadius: 32
        gradientStopPosition4: 1.3
        gradientStopPosition3: 0.9
        gradientStopPosition2: 0.6
        gradientStopPosition1: 0

        Splash_Image25d {
            id: animated_artwork
            x: 358
            y: 0
            width: 442
            height: 480
            clip: true
        }
    }

    Image {
        id: close_window
        x: 779
        y: 5
        width: 13
        height: 13
        fillMode: Image.PreserveAspectFit
        source: "welcome_windows/close.png"
        opacity: area.containsMouse ? 1 : 0.8
        MouseArea {
            id: area
            hoverEnabled: true
            anchors.fill: parent
            anchors.margins: -10
            onClicked: welcome_splash.closeClicked()
        }
    }

    NoShowCheckbox {
        id: do_not_show_checkBox
        x: -47
        y: 430
        padding: 0
        scale: 0.5
    }


    RowLayout {
        x: 16
        y: 254

        visible: welcome_splash.loadingPlugins

        Text {
            id: text1
            color: "#ffffff"
            text: qsTr("%")
            font.pixelSize: 12
            RotationAnimator {
                target: text1
                from: 0
                to: 360
                duration: 1800
                running: true
                loops: -1
            }
        }

        Text {
            id: loading_progress
            color: "#ffffff"
            text: qsTr("Loading Plugins")
            font.family: StudioFonts.titilliumWeb_light
            font.pixelSize: 16
        }

        Text {
            id: text2
            color: "#ffffff"
            text: qsTr("%")
            font.pixelSize: 12
            RotationAnimator {
                target: text2
                from: 0
                to: 360
                duration: 2000
                running: true
                loops: -1
            }
        }
    }

    Text {
        id: all_rights_reserved1
        x: 15
        y: 75
        color: "#ffffff"
        text: qsTr("Community Edition")
        font.pixelSize: 13
        font.family: StudioFonts.titilliumWeb_light
        visible: projectModel.communityVersion
        ProjectModel {
            id: projectModel
        }
    }
}
