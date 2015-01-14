/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0

ButtonRow {
    enabled: anchorBackend.hasParent
    opacity: enabled ? 1 : 0.5

    id: buttonRow

    ButtonRowButton {
        iconSource: "images/anchor-top.png"

        property bool topAnchored: anchorBackend.topAnchored
        onTopAnchoredChanged: {
            checked = topAnchored
        }

        onClicked:  {
            if (checked) {
                if (anchorBackend.bottomAnchored)
                    anchorBackend.verticalCentered = false;
                anchorBackend.topAnchored = true;
            } else {
                anchorBackend.topAnchored = false;
            }
        }
    }

    ButtonRowButton {
        iconSource: "images/anchor-bottom.png"

        property bool bottomAnchored: anchorBackend.bottomAnchored
        onBottomAnchoredChanged: {
            checked = bottomAnchored
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.topAnchored)
                    anchorBackend.verticalCentered = false;
                anchorBackend.bottomAnchored = true;
            } else {
                anchorBackend.bottomAnchored = false;
            }
        }

    }

    ButtonRowButton {
        iconSource: "images/anchor-left.png"

        property bool leftAnchored: anchorBackend.leftAnchored
        onLeftAnchoredChanged: {
            checked = leftAnchored
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.rightAnchored)
                    anchorBackend.horizontalCentered = false;
                anchorBackend.leftAnchored = true;
            } else {
                anchorBackend.leftAnchored = false;
            }
        }
    }

    ButtonRowButton {
        iconSource: "images/anchor-right.png"

        property bool rightAnchored: anchorBackend.rightAnchored
        onRightAnchoredChanged: {
            checked = rightAnchored
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.leftAnchored)
                    anchorBackend.horizontalCentered = false;
                anchorBackend.rightAnchored = true;
            } else {
                anchorBackend.rightAnchored = false;
            }
        }
    }

    ButtonRowButton {
        enabled: false
    }


    ButtonRowButton {
        iconSource: "images/anchor-fill.png"

        property bool isFilled: anchorBackend.isFilled
        onIsFilledChanged: {
            checked = isFilled
        }

        onClicked: {
            if (checked) {
                anchorBackend.fill();
            } else {
                anchorBackend.resetLayout();
            }
        }
    }

    ButtonRowButton {
        enabled: false
    }

    ButtonRowButton {
        iconSource: "images/anchor-vertical.png"

        property bool verticalCentered: anchorBackend.verticalCentered;
        onVerticalCenteredChanged: {
            checked = verticalCentered
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.topAnchored && anchorBackend.bottomAnchored) {
                    anchorBackend.topAnchored = false;
                    anchorBackend.bottomAnchored = false;
                }
                anchorBackend.verticalCentered = true;
            } else {
                anchorBackend.verticalCentered = false;
            }
        }
    }

    ButtonRowButton {
        iconSource: "images/anchor-horizontal.png"

        property bool horizontalCentered: anchorBackend.horizontalCentered;
        onHorizontalCenteredChanged: {
            checked = horizontalCentered
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.leftAnchored && anchorBackend.rightAnchored) {
                    anchorBackend.leftAnchored = false;
                    anchorBackend.rightAnchored = false;
                }
                anchorBackend.horizontalCentered = true;
            } else {
                anchorBackend.horizontalCentered = false;
            }
        }
    }
}
