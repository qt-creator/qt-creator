/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick3D 1.0
import QtQuick.Controls 2.0

Window {
    width: 1024
    height: 768
    visible: true
    title: "3D"
    flags: Qt.WindowStaysOnTopHint | Qt.Window | Qt.WindowTitleHint | Qt.WindowCloseButtonHint

    Rectangle {
        color: "black"
        anchors.fill: parent
    }

    Column {
        y: 32
        Slider {
            id: slider

            value: -600
            from: -1200
            to: 600
        }
        Slider {
            id: slider2

            value: 0
            from: -360
            to: 360
        }
        CheckBox {
            id: checkBox
            text: "Light"
            Rectangle {
                anchors.fill: parent
                z: -1
            }
        }
    }

    Binding {
        target: view.scene
        property: "rotation.y"
        value: slider2.value
    }

    property alias scene: view.scene
    property alias showLight: checkBox.checked

    id: viewWindow

    View3D {
        id: view
        anchors.fill: parent
        enableWireframeMode: true
        camera: camera01

        Light {
            id: directionalLight
            visible: checkBox.checked
        }

        Camera {
            id: camera01
            z: slider.value
        }

        Component.onCompleted: {
            directionalLight.setParentItem(view.scene)
            camera01.setParentItem(view.scene)
        }
    }
}
