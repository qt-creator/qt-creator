// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.4
import CoffeeMachine 1.0

ApplicationFlowForm {
    id: applicationFlow
    state: "initial"

    property int animationDuration: 400

    choosingCoffee.brewButtonSelection.onClicked: {
        applicationFlow.state = "to settings"
        applicationFlow.choosingCoffee.milkSlider.value = applicationFlow.choosingCoffee.sideBar.currentMilk
        applicationFlow.choosingCoffee.sugarSlider.value = 2
    }

    choosingCoffee.sideBar.onCoffeeSelected: {
        applicationFlow.state = "selection"
    }

    choosingCoffee.backButton.onClicked: {
        applicationFlow.state = "back to selection"
    }

    choosingCoffee.brewButton.onClicked: {
        applicationFlow.state = "to empty cup"
    }

    emptyCup.continueButton.onClicked: {
        applicationFlow.state = "to brewing"
        brewing.coffeeName = choosingCoffee.sideBar.currentCoffee
        brewing.foamAmount = choosingCoffee.sideBar.currentFoam
        brewing.milkAmount = applicationFlow.choosingCoffee.milkSlider.value
        brewing.coffeeAmount = choosingCoffee.sideBar.currentCoffeeAmount
        brewing.start()
    }

    brewing.onFinished: {
        applicationFlow.state = "reset"
    }

}
