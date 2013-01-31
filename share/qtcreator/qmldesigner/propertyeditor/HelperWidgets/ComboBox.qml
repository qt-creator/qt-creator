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

QWidget {

    id: comboBox

    property variant backendValue;
    property variant baseStateFlag;
    property alias enabled: box.enabled;

    property alias items: box.items;
    property alias currentText: box.currentText;


    onBaseStateFlagChanged: {
        evaluate();
    }

    property variant isEnabled: comboBox.enabled
    onIsEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (backendValue === undefined)
            return;
        if (!enabled) {
            box.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue.isInModel)
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedBaseColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            } else {
                if (backendValue.isInSubState)
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.changedStateColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
                else
                    box.setStyleSheet("QComboBox,QComboBox:on{color: "+scheme.defaultColor+"}QComboBox:off{color:"+scheme.optionsColor+"}");
            }
        }
    }

    ColorScheme { id:scheme; }

    layout: HorizontalLayout {
        QComboBox {
            id: box
            property variant backendValue: comboBox.backendValue
            onCurrentTextChanged: { backendValue.value = currentText; evaluate(); }
			onItemsChanged: {				
				if (comboBox.backendValue.value == curentText)
				    return;
				box.setCurrentTextSilent(comboBox.backendValue.value);
            }			
			
			property variant backendValueValue: comboBox.backendValue.value
			onBackendValueValueChanged: {			 
			    if (comboBox.backendValue.value == curentText)
				    return;					
				box.setCurrentTextSilent(comboBox.backendValue.value);
			}
            ExtendedFunctionButton {
                backendValue: comboBox.backendValue;
                y: 3
                x: 3
            }
        }
    }
}
