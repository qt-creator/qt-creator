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
import HelperWidgets 1.0

QWidget {
    layout: QVBoxLayout {
        topMargin: 0
        bottomMargin: 0
        leftMargin: 0
        rightMargin: 0
        spacing: 0        
        GroupBox {
            finished: finishedNotify;
            caption: qsTr("Path View")
            layout: VerticalLayout {
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Drag margin")
                        }

                        DoubleSpinBox {
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            backendValue: backendValues.dragMargin
                            minimum: -100;
                            maximum: 100;
                            baseStateFlag: isBaseState;
                        }
                    }
                }
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Flick deceleration")
                        }

                        DoubleSpinBox {
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            backendValue: backendValues.flickDeceleration
                            minimum: 0;
                            maximum: 1000;
                            baseStateFlag: isBaseState;
                        }
                    }
                }
                QWidget {  // 1
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Follows current")
                            toolTip: qsTr("A user cannot drag or flick a PathView that is not interactive.")
                        }
                        CheckBox {
                            backendValue: backendValues.interactive
                            baseStateFlag: isBaseState;
                            checkable: True
                        }
                    }
                }
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Offset")
                            toolTip: qsTr("Specifies how far along the path the items are from their initial positions. This is a real number that ranges from 0.0 to the count of items in the model.")
                        }
                        DoubleSpinBox {
                            alignRight: false
                            spacing: 4
                            singleStep: 1;
                            backendValue: backendValues.offset
                            minimum: 0;
                            maximum: 100;
                            baseStateFlag: isBaseState;
                        }
                    }
                }
                IntEditor {
                    backendValue: backendValues.pathItemCount
                    caption: qsTr("Item count")
                    toolTip: qsTr("pathItemCount: number of items visible on the path at any one time.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: -1;
                    maximumValue: 1000;
                }
            }
        }
        GroupBox {
            finished: finishedNotify;
            caption: qsTr("Path View Highlight")
            layout: VerticalLayout {
                QWidget {
                    layout: HorizontalLayout {
                        Label {
                            text: qsTr("Highlight range")
                        }

                        ComboBox {
                            baseStateFlag: isBaseState
                            items : { ["NoHighlightRange", "ApplyRange", "StrictlyEnforceRange"] }
                            currentText: backendValues.highlightRangeMode.value;
                            onItemsChanged: {
                                currentText =  backendValues.highlightRangeMode.value;
                            }
                            backendValue: backendValues.highlightRangeMode
                        }
                    }
                } //QWidget
                IntEditor {
                    backendValue: backendValues.highlightMoveDuration
                    caption: qsTr("Move duration")
                    toolTip: qsTr("Move animation duration of the highlight delegate.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.preferredHighlightBegin
                    caption: qsTr("Preferred begin")
                    toolTip: qsTr("Preferred highlight begin - must be smaller than Preferred end.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                IntEditor {
                    backendValue: backendValues.preferredHighlightEnd
                    caption: qsTr("Preferred end")
                    toolTip: qsTr("Preferred highlight end - must be larger than Preferred begin.")
                    baseStateFlag: isBaseState;
                    step: 1;
                    minimumValue: 0;
                    maximumValue: 1000;
                }
                QWidget {  // 1
                    layout: HorizontalLayout {

                        Label {
                            text: qsTr("Follows current")
                        }
                        CheckBox {
                            backendValue: backendValues.highlightFollowsCurrentItem
                            toolTip: qsTr("Determines whether the highlight is managed by the view.")
                            baseStateFlag: isBaseState;
                            checkable: True
                        }
                    }
                }
            }
        }
        QScrollArea {
        }
    }
}
