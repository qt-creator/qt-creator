/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.0

ListModel {
    ListElement {
        projectName: "ClusterTutorial"
        qmlFileName: "ClusterTutorial.qml"
        thumbnail: "images/tutorialclusterdemo_thumbnail.png"
        displayName: "Cluster Tutorial"
    }

    ListElement {
        projectName: "CoffeeMachine"
        qmlFileName: "CoffeeMachine.qml"
        thumbnail: "images/coffeemachinedemo_thumbnail.png"
        displayName: "Coffee Machine"
    }

    ListElement {
        projectName: "SideMenu"
        qmlFileName: "SideMenu.qml"
        thumbnail: "images/sidemenu_demo.png"
        displayName: "Side Menu"
    }

    ListElement {
        projectName: "WebinarDemo"
        qmlFileName: "DesignStudioWebinar.qml"
        thumbnail: "images/webinardemo_thumbnail.png"
        displayName: "Webinar Demo"
    }

    ListElement {
        projectName: "EBikeDesign"
        qmlFileName: "EBikeDesign.qml"
        thumbnail: "images/ebike_demo_thumbnail.png"
        displayName: "E-Bike Design"
    }

    ListElement {
        projectName: "ProgressBar"
        qmlFileName: "ProgressBar.ui.qml"
        thumbnail: "images/progressbar_demo.png"
        displayName: "Progress Bar"
    }

    ListElement {
        projectName: "washingMachineUI"
        qmlFileName: "washingMachineUI.qml"
        thumbnail: "images/washingmachinedemo_thumbnail.png"
        displayName: "Washing Machine"
    }

    ListElement {
        projectName: "highendivisystem"
        qmlFileName: "Screen01.ui.qml"
        thumbnail: "images/highendivi_thumbnail.png"
        displayName: "Highend IVI System"
        url: "https://download.qt.io/learning/examples/qtdesignstudio/highendivisystem.zip"
        showDownload: true
    }
}
