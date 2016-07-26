/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.0
import "Constants.js" as Constants

QtObject {
    id: innerObject

    property variant backendValue
    property color textColor: creatorTheme.PanelTextColorLight
    property variant valueFromBackend: backendValue.value;
    property bool baseStateFlag: isBaseState;
    property bool isInModel: backendValue.isInModel;
    property bool isInSubState: backendValue.isInSubState;
    property bool highlight: textColor === __changedTextColor

    property color __defaultTextColor: creatorTheme.PanelTextColorLight
    readonly property color __changedTextColor: creatorTheme.QmlDesigner_HighlightColor

    onBackendValueChanged: {
        evaluate();
    }

    onValueFromBackendChanged: {
        evaluate();
    }

    onBaseStateFlagChanged: {
        evaluate();
    }

    onIsInModelChanged: {
        evaluate();
    }

    onIsInSubStateChanged: {
        evaluate();
    }

    function evaluate() {
        if (innerObject.backendValue === undefined)
            return;

        if (baseStateFlag) {
            if (innerObject.backendValue.isInModel)
                innerObject.textColor = __changedTextColor
            else
                innerObject.textColor = creatorTheme.PanelTextColorLight
        } else {
            if (innerObject.backendValue.isInSubState)
                innerObject.textColor = Constants.colorsChangedStateText
            else
                innerObject.textColor = creatorTheme.PanelTextColorLight
        }

    }
}
