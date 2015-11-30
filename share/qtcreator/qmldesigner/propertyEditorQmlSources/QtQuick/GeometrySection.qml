/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Section {
    caption: qsTr("Geometry")

    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        rowSpacing: 4
        rows: 2

        Label {
            text: qsTr("Position")
        }

        SecondColumnLayout {

            Label {
                text: "X"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.x
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "Y"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.y
                maximumValue: 0xffff
                minimumValue: -0xffff
                decimals: 0
            }
            ExpandingSpacer {

            }
        }
        Label {
            text: qsTr("Size")
        }

        SecondColumnLayout {
            Layout.fillWidth: true

            Label {
                text: "W"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.width
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
            }

            Item {
                width: 4
                height: 4
            }

            Label {
                text: "H"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.height
                maximumValue: 0xffff
                minimumValue: 0
                decimals: 0
            }
            ExpandingSpacer {

            }
        }
    }
}
