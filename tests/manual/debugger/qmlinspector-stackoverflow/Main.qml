// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Reproducer for QTCREATORBUG-33722 / QTCREATORBUG-33740.
//
// This is a minimal QML scene.  The actual crash trigger is in
// main.cpp which injects the engine into the root context's instances
// list and exposes it as a property on the root object.
//
// Debug this project with "Show QML Object Tree" enabled to trigger
// the stack overflow in Qt Creator.

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 300
    visible: true
    title: "QTCREATORBUG-33722 reproducer"

    // The engine is assigned to this property from C++ after loading.
    // This makes it reachable from the inspector's property view.
    property QtObject engineRef

    Column {
        anchors.centerIn: parent
        spacing: 10

        Label {
            text: "Debug this application with Qt Creator"
        }
        Label {
            text: "to trigger the stack overflow."
        }
        Label {
            text: "(Debugger → Show QML Object Tree must be enabled)"
            font.italic: true
        }
    }
}
