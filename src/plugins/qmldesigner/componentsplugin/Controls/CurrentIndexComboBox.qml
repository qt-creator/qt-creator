/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.1 as Controls

SecondColumnLayout {
    Controls.ComboBox {
        id: currentIndexComboBox

        TabViewIndexModel {
            id: tabViewModel
            modelNodeBackendProperty: modelNodeBackend

        }

        property color textColor: currentIndexColorLogic.textColor

        model: tabViewModel.tabViewIndexModel
        Layout.fillWidth: true

        property bool __isCompleted: false

        style: CustomComboBoxStyle {
            textColor: currentIndexComboBox.textColor
        }

        onCurrentTextChanged: {
            if (!__isCompleted)
                return;

            backendValues.currentIndex.value = currentIndexComboBox.currentIndex;
        }

        ColorLogic {
            id: currentIndexColorLogic
            backendValue: backendValues.currentIndex
            onValueFromBackendChanged: {
                invalidate();
            }

            function invalidate() {
                if (currentIndexComboBox.currentIndex !== backendValue.value)
                    currentIndexComboBox.currentIndex = backendValue.value
            }
        }

        Component.onCompleted: {
            currentIndexColorLogic.invalidate()
            __isCompleted = true;
        }

        ExtendedFunctionButton {
            x: 2
            y: 4
            backendValue: backendValues.currentIndex
            visible: currentIndexComboBox.enabled
        }
    }
}
