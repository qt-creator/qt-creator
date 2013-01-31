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
    width:220;fixedWidth:  width
    height:220;fixedHeight: height
    styleSheetFile: "anchorbox.css"

    function isBorderAnchored() {
        return anchorBackend.leftAnchored || anchorBackend.topAnchored || anchorBackend.rightAnchored || anchorBackend.bottomAnchored;
    }

    function fill() {
        anchorBackend.fill();
    }

    function breakLayout() {
        anchorBackend.resetLayout()
    }

    QPushButton {
	text: "fill";
        height:20;fixedHeight: height;
        x: 0;
        y: 0;
        width:200;fixedWidth: width;
        id: qPushButton1;
	onReleased: fill();
    }

    QPushButton {
        text: "break";
	y: 200;
	height:20;fixedHeight: height;
	width:200;fixedWidth: width;
	x: 0;
	id: qPushButton3;
	onReleased: breakLayout();
    }

    QPushButton {
        height:100;fixedHeight: height;
	text: "left";
	font.bold: true;
        x: 16 ;
        y: 60;
        //styleSheet: "border-radius:5px; background-color: #ffda82";
        width:30;fixedWidth: width;
        id: qPushButton5;
	checkable: true;
	checked: anchorBackend.leftAnchored;
	onReleased: {
            if (checked) {
		anchorBackend.horizontalCentered = false;
		anchorBackend.leftAnchored = true;
            } else {
		anchorBackend.leftAnchored = false;
            }
	}
    }

    QPushButton {
	text: "top";
	font.bold: true;
        //styleSheet: "border-radius:5px; background-color: #ffda82";
        height:27;fixedHeight: 27;
        width:100;fixedWidth: 100;
        x: 49;
        y: 30;
        id: qPushButton6;
	checkable: true;
	checked: anchorBackend.topAnchored;
	onReleased: {
            if (checked) {
		anchorBackend.verticalCentered = false;
		anchorBackend.topAnchored = true;
            } else {
		anchorBackend.topAnchored = false;
            }
	}
    }

    QPushButton {
	text: "right";
	font.bold: true;
	x: 153;
	y: 60;
	//styleSheet: "border-radius:5px; background-color: #ffda82";
	width:30;fixedWidth: width;
	height:100;fixedHeight: height;
	id: qPushButton7;
	checkable: true;
	checked: anchorBackend.rightAnchored;
	onReleased: {
            if (checked) {
		anchorBackend.horizontalCentered = false;
		anchorBackend.rightAnchored = true;
            } else {
		anchorBackend.rightAnchored = false;
            }
	}
    }

    QPushButton {
	text: "bottom";
	font.bold: true;
        //styleSheet: "border-radius:5px; background-color: #ffda82";
        width:100;fixedWidth: width;
        x: 49;
        y: 164;
        height:27;fixedHeight: height;
        id: qPushButton8;
	checkable: true;
	checked: anchorBackend.bottomAnchored;
	onReleased: {
            if (checked) {
		anchorBackend.verticalCentered = false;
		anchorBackend.bottomAnchored = true;
            } else {
		anchorBackend.bottomAnchored = false;
            }
	}
    }

    QToolButton {
        width:100;fixedWidth: width;
        //styleSheet: "border-radius:50px;background-color: rgba(85, 170, 255, 255)";
        x: 49;
        y: 60;
        height:100;fixedHeight: height;
        id: qPushButton9;

	QPushButton {
	    width:24;fixedWidth: width;
	    //styleSheet: "border-radius:5px; background-color: #bf3f00;";
	    x: 38;
	    y: 2;
	    height:96;fixedHeight: height;
	    checkable: true;
	    id: horizontalCenterButton;
	    checked: anchorBackend.horizontalCentered;
	    onReleased: {
		if (checked) {
                    anchorBackend.rightAnchored = false;
                    anchorBackend.leftAnchored = false;
                    anchorBackend.horizontalCentered = true;
		} else {
                    anchorBackend.horizontalCentered = false;
		}
	    }
	}

	QPushButton {
	    height:24;fixedHeight: height;
	    x: 2;
	    y: 38;
	    width:96;fixedWidth: width;
	    id: verticalCenterButton;
	    checkable: true;
	    checked: anchorBackend.verticalCentered;
	    onReleased: {
		if (checked) {
                    anchorBackend.topAnchored = false;
                    anchorBackend.bottomAnchored = false;
                    anchorBackend.verticalCentered = true;
		} else {
                    anchorBackend.verticalCentered = false;
		}
	    }
	}

	QPushButton {
	    text: "center";
	    font.bold: true;
	    //styleSheet: "border-radius:20px; background-color: #ff5500";
	    width:40;fixedWidth: width;
	    height:40;fixedHeight: height;
	    x: 30;
	    y: 30;
	    id: centerButton;
	    checkable: true;
	    checked: anchorBackend.verticalCentered && anchorBackend.horizontalCentered;
	    onReleased: {
                if (checked) {
		    anchorBackend.leftAnchored = false;
		    anchorBackend.topAnchored = false;
		    anchorBackend.rightAnchored = false;
		    anchorBackend.bottomAnchored = false;
		    anchorBackend.verticalCentered = true;
		    anchorBackend.horizontalCentered = true;
                } else {
		    anchorBackend.verticalCentered = false;
		    anchorBackend.horizontalCentered = false;
                }
	    }
	}
    }
}
