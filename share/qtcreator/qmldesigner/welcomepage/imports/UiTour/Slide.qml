// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Timeline

Item {
    id: root
    width: 1920
    height: 1080

    property string caption: "This is a string"
    property string title: "this is a string"
    property bool active: false

    function prev() {
        var states = root.stateNames()

        if (states.length === 0)
           return false

        if (root.state === "")
            return false

        var index = states.indexOf(root.state)

        // base state is not in the list
        if (index > 0) {
            root.state = states[index - 1]
            return true
        }

        return false
    }

    function next() {
        var states = root.stateNames()

        if (states.length === 0)
           return false

        if (root.state === "") {
            root.state = states[0]
            return true
        }

        var index = states.indexOf(root.state)

        if (index < (states.length - 1)) {
            root.state = states[index + 1]
            return true
        }

        return false
    }

    function stateNames() {
        var states = []

        for (var i = 0; i < root.states.length; i++) {
            var state = root.states[i]
            states.push(state.name)
        }

        return states
    }

    signal activated

    function activate() {
        root.active = true
        stateGroup.state = "active"
        root.activated()
    }

    function done() {
        stateGroup.state = "done"
    }

    function init() {
        root.active = false
        stateGroup.state = "inactive"
    }

    StateGroup {
        id: stateGroup
        states: [
            State {
                name: "active"

                PropertyChanges {
                    target: root
                    opacity: 1
                    visible: true
                }
            },
            State {
                name: "inactive"

                PropertyChanges {
                    target: root
                    opacity: 0
                    visible: true
                }
            },
            State {
                name: "done"

                PropertyChanges {
                    target: root
                    opacity: 1
                    visible: true
                }
            }
        ]
    }
}
