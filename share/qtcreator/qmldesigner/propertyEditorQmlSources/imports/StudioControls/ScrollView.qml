/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

T.ScrollView {
    id: control

    property alias horizontalThickness: horizontalScrollBar.height
    property alias verticalThickness: verticalScrollBar.width
    property bool bothVisible: verticalScrollBar.visible
                               && horizontalScrollBar.visible

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    ScrollBar.vertical: ScrollBar {
        id: verticalScrollBar
        parent: control
        x: control.width - verticalScrollBar.width - StudioTheme.Values.border
        y: StudioTheme.Values.border
        height: control.availableHeight - (2 * StudioTheme.Values.border)
                - (control.bothVisible ? control.horizontalThickness : 0)
        active: control.ScrollBar.horizontal.active
    }

    ScrollBar.horizontal: ScrollBar {
        id: horizontalScrollBar
        parent: control
        x: StudioTheme.Values.border
        y: control.height - horizontalScrollBar.height - StudioTheme.Values.border
        width: control.availableWidth - (2 * StudioTheme.Values.border)
               - (control.bothVisible ? control.verticalThickness : 0)
        active: control.ScrollBar.vertical.active
    }
}
