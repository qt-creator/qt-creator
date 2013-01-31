/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0
import Bauhaus 1.0

QWidget { //This is a special checkBox that does color coding for states

    id: checkBox;

    property variant backendValue;

    property variant baseStateFlag;
    property alias checkable: localCheckBox.checkable
    property alias text: localLabel.text

    property alias checked: localCheckBox.checked

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    property bool isInModel: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    function evaluate() {
        if (!enabled) {
            localLabel.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue.isInModel)
                    localLabel.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    localLabel.setStyleSheet("color: "+scheme.boldTextColor);
            } else {
                if (backendValue.isInSubState)
                    localLabel.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    localLabel.setStyleSheet("color: "+scheme.boldTextColor);
            }
        }
    }

    ColorScheme { id:scheme; }


    layout: HorizontalLayout {
        spacing: 4

        QCheckBox {
            id: localCheckBox
            checkable: true;
            checked: backendValue.value;
            onToggled: {
                backendValue.value = checked;
            }
            maximumWidth: 30
        }

        QLabel {
            id: localLabel
            font.bold: true;
            alignment: "Qt::AlignLeft | Qt::AlignVCenter"
        }

    }


    ExtendedFunctionButton {
        backendValue: checkBox.backendValue
        y: 3
        x: localCheckBox.x + 18;
    }
}

