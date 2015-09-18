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
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Text")

    property bool showIsWrapping: false
    property bool showVerticalAlignment: false
    property bool useLineEdit: true
    property bool showFormatProperty: false

    SectionLayout {
        columns: 2
        rows: 3
        Label {
            text: qsTr("Text")
        }
        LineEdit {
            //visible: useLineEdit
            backendValue: backendValues.text
            Layout.fillWidth: true
        }

        Label {
            visible: showVerticalAlignment
            text: qsTr("Wrap mode")
        }

        ComboBox {
            visible: showVerticalAlignment
            Layout.fillWidth: true
            backendValue: backendValues.wrapMode
            scope: "Text"
            model: ["NoWrap", "WordWrap", "WrapAnywhere", "WrapAtWordBoundaryOrAnywhere"]
        }

        Label {
            text: qsTr("Alignment")
        }

        AligmentHorizontalButtons {

        }

        Label {
            visible: showVerticalAlignment
            text: ("")
        }

        AligmentVerticalButtons {
            visible: showVerticalAlignment
        }


        Label {
            visible: showFormatProperty
            text: qsTr("Format")
        }
        ComboBox {
            scope: "Text"
            visible: showFormatProperty
            model:  ["PlainText", "RichText", "AutoText"]
            backendValue: backendValues.textFormat
            Layout.fillWidth: true
        }
    }
}
