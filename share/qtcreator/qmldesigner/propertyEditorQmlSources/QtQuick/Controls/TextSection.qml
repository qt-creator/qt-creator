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

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioTheme 1.0 as StudioTheme

Section {
    width: parent.width
    caption: qsTr("Text Area")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Placeholder text")
            tooltip: qsTr("Placeholder text displayed when the editor is empty.")
        }

        SecondColumnLayout {
            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.placeholderText
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Placeholder color")
            tooltip: qsTr("Placeholder text color.")
        }

        ColorEditor {
            backendValue: backendValues.placeholderTextColor
            supportGradient: false
        }

        PropertyLabel {
            text: qsTr("Hover")
            tooltip: qsTr("Whether text area accepts hover events.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.hoverEnabled.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.hoverEnabled
            }

            ExpandingSpacer {}
        }
    }
}
