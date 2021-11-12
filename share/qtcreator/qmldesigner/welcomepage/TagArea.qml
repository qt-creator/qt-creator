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

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0
import QtQuick.Layouts 1.0

Item {
    id: tagArea
    width: 200
    height: 15

    RowLayout {
        x: 0
        y: 0
        width: 220
        height: 15


        TagPageButton {
            id: tagPageButtonLeft
        }

        Tag {
            id: tag1
            Layout.preferredWidth: 50
        }

        Tag {
            id: tag2
            Layout.preferredWidth: 50
        }

        Tag {
            id: tag3
            Layout.preferredWidth: 50
        }

        TagPageButton {
            id: tagPageButtonRight
            rotation: 180
        }

    }

}

/*##^##
Designer {
    D{i:0;height:15;width:220}D{i:5}D{i:6}
}
##^##*/
