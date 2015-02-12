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
