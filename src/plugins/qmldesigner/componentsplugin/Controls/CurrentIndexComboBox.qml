// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
