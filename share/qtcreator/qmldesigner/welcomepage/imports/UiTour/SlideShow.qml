// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    width: 1920
    height: 1080
    color: "#00000000"

    property string caption
    property string title

    property int progress: 0
    property int currentSlide: 0

    function next() {
        if (root.currentSlide === (root.progress - 1))
            return

        var index = root.findActive()
        var current = root.children[index]

        root.currentSlide++

        if (current.next()) {
            root.caption = current.caption
            root.title = current.title
            return
        }

        root.children[index].init()
        root.children[index + 1].activate()

        root.caption = root.children[index + 1].caption
        root.title = root.children[index + 1].title
    }

    function prev() {
        if (root.currentSlide === 0)
            return

        var index = root.findActive()
        var current = root.children[index]

        root.currentSlide--

        if (current.prev()) {
            root.caption = current.caption
            root.title = current.title
            return
        }

        root.children[index].init()
        root.children[index - 1].activate()
        root.caption = root.children[index - 1].caption
        root.title = root.children[index - 1].title
    }

    function findActive() {
        for (var i = 0; i < root.children.length; i++) {
            var child = root.children[i]
            if (child.active)
                return i
        }
        return -1
    }

    Component.onCompleted: {
        for (var i = 0; i < root.children.length; i++) {
            var child = root.children[i]
            child.init()
            root.progress += child.states.length
            if (i === 0) {
                child.visible = true
                child.activate()
            }
        }

        root.caption = root.children[0].caption
        root.title = root.children[0].title
    }
}
