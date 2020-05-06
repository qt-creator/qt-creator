/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Design Studio.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

import QtQuick 2.4

SideBarForm {
    id: sideBar
    property string currentCoffee: qsTr("Cappuccino")
    signal coffeeSelected
    property real currentFoam: 1
    property real currentMilk: 0
    property real currentCoffeeAmount: 4

    Behavior on currentMilk {
         NumberAnimation { duration: 250 }
    }

    Behavior on currentCoffeeAmount {
         NumberAnimation { duration: 250 }
    }

    macchiatoButton.onClicked: {
        sideBar.currentCoffee = qsTr("Macchiato")
        sideBar.currentFoam = 1
        sideBar.currentMilk = 1
        sideBar.currentCoffeeAmount = 4
        sideBar.coffeeSelected()
    }

    latteButton.onClicked: {
        sideBar.currentCoffee = qsTr("Latte")
        sideBar.currentFoam = 1
        sideBar.currentMilk = 10
        sideBar.currentCoffeeAmount = 3
        sideBar.coffeeSelected()
    }

    espressoButton.onClicked: {
        sideBar.currentCoffee = qsTr("Espresso")
        sideBar.currentFoam = 0
        sideBar.currentMilk = 0
        sideBar.currentCoffeeAmount = 4
        sideBar.coffeeSelected()
    }

    cappuccinoButton.onClicked: {
        sideBar.currentCoffee = qsTr("Cappuccino")
        sideBar.currentFoam = 1
        sideBar.currentMilk = 7
        sideBar.currentCoffeeAmount = 3.5
        sideBar.coffeeSelected()
    }
}
