QWidget {
    layout: HorizontalLayout {
        Label {
            text: "%1"
            toolTip: "%1"
        }
        UrlEdit {
            backendValue: backendValues.%2
            baseStateFlag: isBaseState
        }
    }
}
