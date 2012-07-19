/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

import QtQuick 1.0

// KNOWN ISSUES
// none

Item {
    id: popupBehavior

    property bool showing: false
    property bool whenAlso: true            // modifier to the "showing" property
    property bool consumeCancelClick: true
    property int delay: 0                   // delay before popout becomes visible
    property int deallocationDelay: 3000    // 3 seconds

    property Component popupComponent

    property alias popup: popupLoader.item  // read-only
    property alias window: popupBehavior.root // read-only

    signal prepareToShow
    signal prepareToHide
    signal cancelledByClick

    // implementation

    anchors.fill: parent

    onShowingChanged: notifyChange()
    onWhenAlsoChanged: notifyChange()
    function notifyChange() {
        if(showing && whenAlso) {
            if(popupLoader.sourceComponent == undefined) {
                popupLoader.sourceComponent = popupComponent;
            }
        } else {
            mouseArea.enabled = false; // disable before opacity is changed in case it has fading behavior
            if(Qt.isQtObject(popupLoader.item)) {
                popupBehavior.prepareToHide();
                popupLoader.item.opacity = 0;
            }
        }
    }

    property Item root: findRoot()
    function findRoot() {
        var p = parent;
        while(p.parent != undefined)
            p = p.parent;

        return p;
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: false  // enabled only when popout is showing
        onPressed: {
            popupBehavior.showing = false;
            mouse.accepted = consumeCancelClick;
            cancelledByClick();
        }
    }

    Loader {
        id: popupLoader
    }

    Timer { // visibility timer
        running: Qt.isQtObject(popupLoader.item) && showing && whenAlso
        interval: delay
        onTriggered: {
            popupBehavior.prepareToShow();
            mouseArea.enabled = true;
            popup.opacity = 1;
        }
    }

    Timer { // deallocation timer
        running: Qt.isQtObject(popupLoader.item) && popupLoader.item.opacity == 0
        interval: deallocationDelay
        onTriggered: popupLoader.sourceComponent = undefined
    }

    states: State {
        name: "active"
        when: Qt.isQtObject(popupLoader.item) && popupLoader.item.opacity > 0
        ParentChange { target: popupBehavior; parent: root }
    }
 }

