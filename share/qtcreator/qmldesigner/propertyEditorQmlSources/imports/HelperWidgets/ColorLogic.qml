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
import StudioTheme 1.0 as StudioTheme

QtObject {
    id: root

    property variant backendValue
    property color textColor: StudioTheme.Values.themeTextColor
    property variant valueFromBackend: root.backendValue === undefined ? 0 : root.backendValue.value
    property bool baseStateFlag: isBaseState
    property bool isInModel: {
        if (root.backendValue !== undefined && root.backendValue.isInModel !== undefined)
            return root.backendValue.isInModel

        return false
    }
    property bool isInSubState: {
        if (root.backendValue !== undefined && root.backendValue.isInSubState !== undefined)
            return root.backendValue.isInSubState

        return false
    }
    property bool highlight: root.textColor === root.__changedTextColor
    property bool errorState: false

    readonly property color __defaultTextColor: StudioTheme.Values.themeTextColor
    readonly property color __changedTextColor: StudioTheme.Values.themeInteraction
    readonly property color __errorTextColor: StudioTheme.Values.themeError

    onBackendValueChanged: root.evaluate()
    onValueFromBackendChanged: root.evaluate()
    onBaseStateFlagChanged: root.evaluate()
    onIsInModelChanged: root.evaluate()
    onIsInSubStateChanged: root.evaluate()
    onErrorStateChanged: root.evaluate()

    function evaluate() {
        if (root.backendValue === undefined)
            return

        if (root.errorState) {
            root.textColor = root.__errorTextColor
            return
        }

        if (root.baseStateFlag) {
            if (root.backendValue.isInModel)
                root.textColor = root.__changedTextColor
            else
                root.textColor = root.__defaultTextColor
        } else {
            if (root.backendValue.isInSubState)
                root.textColor = StudioTheme.Values.themeChangedStateText
            else
                root.textColor = root.__defaultTextColor
        }
    }
}
