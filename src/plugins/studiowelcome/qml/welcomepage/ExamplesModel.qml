/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
        qmlFileName: "content/Cluster_Art.ui.qml"
        thumbnail: "images/tutorialclusterdemo_thumbnail.png"
        displayName: "Cluster Tutorial"
        showDownload: false
    }

    ListElement {
        projectName: "CoffeeMachine"
        qmlFileName: "content/ApplicationFlowForm.ui.qml"
        thumbnail: "images/coffeemachinedemo_thumbnail.png"
        displayName: "Coffee Machine"
        showDownload: false
    }

    ListElement {
        projectName: "SideMenu"
        qmlFileName: "content/MainForm.ui.qml"
        thumbnail: "images/sidemenu_demo.png"
        displayName: "Side Menu"
        showDownload: false
    }

    ListElement {
        projectName: "WebinarDemo"
        qmlFileName: "content/MainApp.ui.qml"
        thumbnail: "images/webinardemo_thumbnail.png"
        displayName: "Webinar Demo"
        showDownload: false
    }

    ListElement {
        projectName: "EBikeDesign"
        qmlFileName: "content/Screen01.ui.qml"
        thumbnail: "images/ebike_demo_thumbnail.png"
        displayName: "E-Bike Design"
        showDownload: false
    }

    ListElement {
        projectName: "ProgressBar"
        qmlFileName: "content/ProgressBar.ui.qml"
        thumbnail: "images/progressbar_demo.png"
        displayName: "Progress Bar"
        showDownload: false
    }

    ListElement {
        projectName: "washingMachineUI"
        qmlFileName: "washingMachineUI.qml"
        thumbnail: "images/washingmachinedemo_thumbnail.png"
        displayName: "Washing Machine"
        showDownload: false
    }

    ListElement {
        projectName: "SimpleKeyboard"
        qmlFileName: "SimpleKeyboard.qml"
        thumbnail: "images/virtualkeyboard_thumbnail.png"
        displayName: "Virtual Keyboard"
        showDownload: false
    }

    ListElement {
        projectName: "highendivisystem"
        qmlFileName: "Screen01.ui.qml"
        thumbnail: "images/highendivi_thumbnail.png"
        displayName: "IVI System"
        url: "https://download.qt.io/learning/examples/qtdesignstudio/highendivisystem.zip"
        showDownload: true
    }

    ListElement {
        projectName: "digitalcluster"
        qmlFileName: "Screen01.ui.qml"
        thumbnail: "images/digital_cluster_thumbnail.png"
        displayName: "Digital Cluster"
        url: "https://download.qt.io/learning/examples/qtdesignstudio/digitalcluster.zip"
        showDownload: true
    }

    ListElement {
        projectName: "effectdemo"
        qmlFileName: "Screen01.ui.qml"
        thumbnail: "images/effectdemo_thumbnail.png"
        displayName: "Effect Demo"
        url: "https://download.qt.io/learning/examples/qtdesignstudio/effectdemo.zip"
        showDownload: true
    }


    ListElement {
        projectName: "cppdemoproject"
        explicitQmlproject: "qml/qdsproject.qmlproject"
        qmlFileName: "WashingMachineHome/MainFile.ui.qml"
        thumbnail: "images/cppdemo_thumbnail.png"
        displayName: "C++ Demo Project"
        url: "https://download.qt.io/learning/examples/qtdesignstudio/cppdemoproject.zip"
        showDownload: true
    }

    ListElement {
        projectName: "particles"
        qmlFileName: "Fire.ui.qml"
        thumbnail: "images/particles_thumbnail.png"
        displayName: "Particle Demo"
        url: "https://download.qt.io/learning/examples/qtdesignstudio/particles.zip"
        showDownload: true
    }

    ListElement {
        projectName: "thermo"
        qmlFileName: "thermo.ui.qml"
        thumbnail: "images/thermo_thumbnail.png"
        displayName: "Thermostat Demo"
        url: "https://download.qt.io/learning/examples/qtdesignstudio/thermo.zip"
        showDownload: true
    }
}
