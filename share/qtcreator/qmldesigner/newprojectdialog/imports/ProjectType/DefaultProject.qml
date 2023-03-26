// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick.Window
import QtQuick.Controls

import QtQuick
import QtQuick.Layouts

import NewProjectDialog

Item {
    anchors.fill: parent

    Row {
        anchors.fill: parent

        Details {
            height: parent.height
        }

        Styles {
            height: parent.height
        }
    }
}
