/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
