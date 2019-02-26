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
        qmlFileName: "Cluster_Art.ui.qml"
        thumbnail: "images/tutorialclusterdemo_thumbnail.png"
        displayName: "Cluster Tutorial"
    }

    ListElement {
        projectName: "CoffeeMachine"
        qmlFileName: "ApplicationFlowForm.ui.qml"
        thumbnail: "images/coffeemachinedemo_thumbnail.png"
        displayName: "Coffee Machine"
    }

    ListElement {
        projectName: "SideMenu"
        qmlFileName: "MainFile.ui.qml"
        thumbnail: "images/sidemenu_demo.png"
        displayName: "Side Menu"
    }

    ListElement {
        projectName: "WebinarDemo"
        qmlFileName: "MainApp.ui.qml"
        thumbnail: "images/webinardemo_thumbnail.png"
        displayName: "Webinar Demo"
    }

    ListElement {
        projectName: "EBikeDesign"
        qmlFileName: "Screen01.ui.qml"
        thumbnail: "images/ebike_demo_thumbnail.png"
        displayName: "E-Bike Design"
    }

    ListElement {
        projectName: "ProgressBar"
        qmlFileName: "ProgressBar.ui.qml"
        thumbnail: "images/progressbar_demo.png"
        displayName: "Progress Bar"
    }
}
