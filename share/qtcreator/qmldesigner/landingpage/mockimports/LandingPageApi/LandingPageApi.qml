// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick 2.15
import StudioFonts

QtObject {
    property bool qdsInstalled: true
    property bool projectFileExists: true
    property string qtVersion: "6.1"
    property string qdsVersion: "3.6"

    function openQtc(rememberSelection) { console.log("openQtc", rememberSelection) }
    function openQds(rememberSelection) { console.log("openQds", rememberSelection) }
    function installQds() { console.log("installQds") }
    function generateProjectFile() { console.log("generateProjectFile") }

    // This property ensures that the Titillium font will be loaded and
    // can be used by the theme.
    property string family: StudioFonts.titilliumWeb_regular
}
