/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts
import StudioControls 1.0 as StudioControls
import projectmodel 1.0

Item {
    id: appFrame
    clip:  true
    width: Constants.width
    height: Constants.height
    MainScreen {
        id: screen
        anchors.fill: parent
        anchors.leftMargin: screen.designMode ? 0 : -45 //hide sidebar
    }

    property int pageIndex: 0
    property int minimumWidth: 1200
    property int minimumHeight: 720

    onHeightChanged: {
        if (appFrame.height > appFrame.minimumHeight) anchors.fill = parent
        else if (appFrame.height < appBackground.minimumHeight) appFrame.height = appFrame.minimumHeight
        console.log ("checked min height " + appFrame.height)
    }
    onWidthChanged: {
        if (appFrame.width > appFrame.minimumWidth) anchors.fill = parent
        else if (appFrame.width < appFrame.minimumWidth) appFrame.width = appFrame.minimumWidth
        console.log ("checked min width " + appFrame.width)
    }
}
