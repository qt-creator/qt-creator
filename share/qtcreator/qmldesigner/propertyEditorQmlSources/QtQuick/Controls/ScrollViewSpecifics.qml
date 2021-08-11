/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("Scroll View")

        SectionLayout {
            PropertyLabel { text: qsTr("Content size") }

            SecondColumnLayout {
                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.contentWidth
                    minimumValue: 0
                    maximumValue: 10000
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The width of the object
                    text: qsTr("W", "width")
                    tooltip: qsTr("Content width used for calculating the total implicit width.")
                }

                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                SpinBox {
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                    backendValue: backendValues.contentHeight
                    minimumValue: 0
                    maximumValue: 10000
                }

                Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                ControlLabel {
                    //: The height of the object
                    text: qsTr("H", "height")
                    tooltip: qsTr("Content height used for calculating the total implicit height.")
                }
/*
                TODO QDS-4836
                Spacer { implicitWidth: StudioTheme.Values.controlGap }

                LinkIndicator2D {}
*/
                ExpandingSpacer {}
            }
        }
    }

    ControlSection {}

    PaddingSection {}

    FontSection {
        caption: qsTr("Font Inheritance")
        expanded: false
    }
}
