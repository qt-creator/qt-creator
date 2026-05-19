// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlJSTools::Internal {

constexpr char previewText[] = R"(import QtQuick
import QtQuick.Templates
import QtQuick.Controls
Rectangle {
    // Object
    MouseArea {
        onPressed: {
            console.log("pressed");;;
        }
        anchors.fill: parent
    }

    id: myId

    // Binding
    height: myHeight

    // Function
    function computeValue(x, y) {
        return x + y;;
    }

    // Property (scattered)
    property int z: 3

    // Signal
    signal triggered

    // Another object
    Item {
        id: childItem
        width: 50
        height: 50
    }

    // Required property
    required property int requiredValue

    // Alias
    property alias childRef: childItem

    // Inline binding with JS
    width: computeValue(100, 200)

    // Enum
    enum Mode {
        ModeA,
        ModeB,
        ModeC
    }

    // Another function
    function f() {
        ;;
    }

    // Attached property
    Component.onCompleted: {
        console.log("Completed");;;
    }

    // Readonly property
    readonly property int constValue: 42

    // Another signal
    signal valueChanged(int newValue)

    // Grouped property (font)
    property font myFont: Qt.font({
        family: "Arial",
        pixelSize: 14
    })

    // Another object (empty)
    QtObject {
    }

    // Binding using ternary
    opacity: enabled ? 1.0 : 0.5

    // More properties (intentionally late)
    property bool enabled: true
    property string name: "qmlformat"

    // Function using arrow syntax
    function iterate() {
        [1,2,3].forEach(x => console.log(x));;;
    }

    // Signal handler style
    onTriggered: {
        console.log("triggered handler")
    }

    // Another attached property
    Component.onDestruction: {
        console.log("Destroyed")
    }

    // Nested object with mixed content
    Text {
        text: name
        anchors.centerIn: parent

        Component.onCompleted: {
            console.log("Text ready");;
        }
    }

    // More bindings
    y: 20
    x: 10

    // Another enum (tests grouping behavior)
    enum Status {
        Idle,
        Running,
        Stopped
    }

    // Complex JS binding
    property var data: ({
        a: 1,
        b: 2,
        c: [1,2,3]
    })

    // Another function after everything
    function zFunc() {
        let tmp = 0;;;
        for (let i = 0; i < 5; ++i) {
            tmp += i;
        }
        return tmp;
    }
})";

} // namespace QmlJSTools::Internal
