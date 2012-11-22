QWidget {
    layout: HorizontalLayout {
        Label {
            text: "%1"
            toolTip: "%1"
        }
        CheckBox {
            text: backendValues.%2.value
            backendValue: backendValues.%2
            baseStateFlag: isBaseState
            checkable: true
       }
    }
}