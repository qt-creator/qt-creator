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

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.ScrollBar {
    id: scrollBar

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentItem: Rectangle {
        implicitWidth: scrollBar.interactive ? 14 : 8
        implicitHeight: scrollBar.interactive ? 14 : 8
        radius: width / 2
        opacity: 0.0
        color: scrollBar.pressed ? StudioTheme.Values.themeScrollBarHandle //"#4C4C4C"//DARK
                                 : StudioTheme.Values.themeScrollBarHandle //"#3E3E3E"//DARK

        states: State {
            name: "active"
            when: scrollBar.active && scrollBar.size < 1.0
            PropertyChanges {
                target: scrollBar.contentItem
                opacity: 0.9
            }
        }

        transitions: Transition {
            from: "active"
            SequentialAnimation {
                PauseAnimation {
                    duration: 450
                }
                NumberAnimation {
                    target: scrollBar.contentItem
                    duration: 200
                    property: "opacity"
                    to: 0.0
                }
            }
        }
    }
}
