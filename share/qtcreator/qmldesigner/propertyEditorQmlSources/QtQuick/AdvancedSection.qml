/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Advanced")

    SectionLayout {
        rows: 4

        Label {
            text: qsTr("Origin")
        }

        OriginControl {
            backendValue: backendValues.transformOrigin
        }

        Label {
            text: qsTr("Scale")
        }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.scale
                hasSlider: true
                decimals: 2
                minimumValue: 0.01
                stepSize: 0.1
                maximumValue: 10
                Layout.preferredWidth: 80
            }
            ExpandingSpacer {
            }
        }
        Label {
            text: qsTr("Rotation")
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.rotation
                hasSlider: true
                decimals: 2
                minimumValue: -360
                maximumValue: 360
                Layout.preferredWidth: 80
            }
            ExpandingSpacer {
            }
        }
        Label {
            text: "Z"
        }
        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.z
                hasSlider: true
                minimumValue: -100
                maximumValue: 100
                Layout.preferredWidth: 80
            }
            ExpandingSpacer {
            }
        }

        Label {
            visible: majorQtQuickVersion > 1
            text: qsTr("Enabled")
        }
        SecondColumnLayout {
            visible: majorQtQuickVersion > 1
            CheckBox {
                backendValue: backendValues.enabled
                text: qsTr("Accept mouse and keyboard events")
            }
            ExpandingSpacer {
            }
        }

        Label {
            visible: majorQtQuickVersion > 1
            text: qsTr("Smooth")
        }
        SecondColumnLayout {
            visible: majorQtQuickVersion > 1
            CheckBox {
                backendValue: backendValues.smooth
                text: qsTr("Smooth sampling active")
            }
            ExpandingSpacer {
            }
        }

        Label {
            visible: majorQtQuickVersion > 1
            text: qsTr("Antialiasing")
        }
        SecondColumnLayout {
            visible: majorQtQuickVersion > 1
            CheckBox {
                backendValue: backendValues.antialiasing
                text: qsTr("Anti-aliasing active")
            }
            ExpandingSpacer {
            }
        }

    }
}
