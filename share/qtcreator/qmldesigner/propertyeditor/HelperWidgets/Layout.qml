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

GroupBox {
    finished: finishedNotify;

    caption: qsTr("Layout")

    id: layout;
    enabled: anchorBackend.hasParent;

    property bool isInBaseState: isBaseState

    property variant targetLabelWidth: 60
    property int leftMarginMargin: 16 + 10 + 10
    layout: VerticalLayout {
        Label {
            text: qsTr("Anchors")
        }
        QWidget {
            layout: HorizontalLayout {
                leftMargin: 10
                topMargin: 8


                AnchorButtons {
                    opacity: enabled ? 1.0 : 0.3;
                    enabled: isInBaseState
                    fixedWidth:266
                    toolTip: enabled ? qsTr("Set anchors") : qsTr("Setting anchors in states is not supported.")
                }
            }
        }

        QWidget {
            visible: anchorBackend.topAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-top.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.topTarget
                            onSelectedItemNodeChanged: { anchorBackend.topTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        Label {
                            text: qsTr("Margin")
                            fixedWidth: targetLabelWidth
                        }
                        IntEditor {
                            id:topbox
                            slider: false
                            caption: ""
                            backendValue: backendValues.anchors_topMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 999
                            minimumValue: -999
                            step: 1
                        }

                        PlaceHolder {
                            fixedWidth: 80
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.bottomAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-bottom.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.bottomTarget
                            onSelectedItemNodeChanged: { anchorBackend.bottomTarget = selectedItemNode; }
                        }

                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        Label {
                            text: qsTr("Margin")
                            fixedWidth: targetLabelWidth
                        }
                        IntEditor {
                            slider: false
                            caption: ""
                            backendValue: backendValues.anchors_bottomMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 999
                            minimumValue: -999
                            step: 1
                        }

                        PlaceHolder {
                            fixedWidth: 80
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.leftAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-left.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.leftTarget
                            onSelectedItemNodeChanged: { anchorBackend.leftTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        Label {
                            text: qsTr("Margin")
                            fixedWidth: targetLabelWidth
                        }
                        IntEditor {
                            slider: false
                            caption: ""
                            backendValue: backendValues.anchors_leftMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 999
                            minimumValue: -999
                            step: 1
                        }

                        PlaceHolder {
                            fixedWidth: 80
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.rightAnchored;
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-right.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.rightTarget
                            onSelectedItemNodeChanged: { anchorBackend.rightTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        Label {
                            text: qsTr("Margin")
                            fixedWidth: targetLabelWidth
                        }
                        IntEditor {
                            slider: false
                            caption: ""
                            backendValue: backendValues.anchors_rightMargin
                            baseStateFlag: isInBaseState;
                            maximumValue: 999
                            minimumValue: -999
                            step: 1
                        }

                        PlaceHolder {
                            fixedWidth: 80
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.horizontalCentered
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-horizontal.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.horizontalTarget
                            onSelectedItemNodeChanged: { anchorBackend.horizontalTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        Label {
                            text: qsTr("Margin")
                            fixedWidth: targetLabelWidth
                        }
                        IntEditor {
                            slider: false
                            caption: ""
                            baseStateFlag: isInBaseState;
                            backendValue: backendValues.anchors_horizontalCenterOffset
                            maximumValue: 999
                            minimumValue: -999
                            step: 1
                        }

                        PlaceHolder {
                            fixedWidth: 80
                        }
                    }
                }
            }
        }
        QWidget {
            visible: anchorBackend.verticalCentered
            layout : VerticalLayout {
                topMargin: 8;
                bottomMargin: 4;
                rightMargin: 10;
                QWidget {
                    layout: HorizontalLayout {
                        leftMargin: 10
                        QLabel {
                            //iconFromFile: "qrc:qmldesigner/images/icon-top.png"
                            fixedWidth: 16
                            fixedHeight: 16
                            styleSheet: "border-image: url(:/qmldesigner/images/icon-vertical.png)";
                        }

                        Label {
                            text: qsTr("Target")
                            fixedWidth: targetLabelWidth
                        }
                        SiblingComboBox {
                            enabled: isInBaseState
                            itemNode: anchorBackend.itemNode
                            selectedItemNode: anchorBackend.verticalTarget
                            onSelectedItemNodeChanged: { anchorBackend.verticalTarget = selectedItemNode; }
                        }
                    }
                }

                QWidget {
                    layout : HorizontalLayout {
                        leftMargin: leftMarginMargin
                        Label {
                            text: qsTr("Margin")
                            fixedWidth: targetLabelWidth
                        }
                        IntEditor {
                            slider: false
                            caption: ""
                            backendValue: backendValues.anchors_verticalCenterOffset
                            baseStateFlag: isInBaseState;
                            maximumValue: 999
                            minimumValue: -999
                            step: 1
                        }

                        PlaceHolder {
                            fixedWidth: 80
                        }
                    }
                }
            }
        }
    }
}
