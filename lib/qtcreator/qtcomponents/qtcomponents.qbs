import qbs.base 1.0

Product {
    type: ["installed_content"]
    name: "QtComponents"

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/"
        fileTags: ["install"]
        files: [
            "Button.qml",
            "ButtonRow.qml",
            "CheckBox.qml",
            "ChoiceList.qml",
            "ContextMenu.qml",
            "Dial.qml",
            "Frame.qml",
            "GroupBox.qml",
            "Menu.qml",
            "MenuItem.qml",
            "ProgressBar.qml",
            "RadioButton.qml",
            "ScrollArea.qml",
            "ScrollBar.qml",
            "Slider.qml",
            "SpinBox.qml",
            "SplitterRow.qml",
            "Switch.qml",
            "Tab.qml",
            "TabBar.qml",
            "TabFrame.qml",
            "TableColumn.qml",
            "TableView.qml",
            "TextArea.qml",
            "TextField.qml",
            "ToolBar.qml",
            "ToolButton.qml",
            "qmldir",
        ]
    }

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/custom"
        fileTags: ["install"]
        files: [
            "custom/BasicButton.qml",
            "custom/Button.qml",
            "custom/ButtonColumn.qml",
            "custom/ButtonGroup.js",
            "custom/ButtonRow.qml",
            "custom/CheckBox.qml",
            "custom/ChoiceList.qml",
            "custom/GroupBox.qml",
            "custom/ProgressBar.qml",
            "custom/Slider.qml",
            "custom/SpinBox.qml",
            "custom/SplitterRow.qml",
            "custom/TextField.qml",
            "custom/qmldir",
        ]
    }

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/custom/behaviors"
        fileTags: ["install"]
        files: [
            "custom/behaviors/ButtonBehavior.qml",
            "custom/behaviors/ModalPopupBehavior.qml",
        ]
    }

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/custom/private"
        fileTags: ["install"]
        files: [
            "custom/private/ChoiceListPopup.qml",
        ]
    }

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/images"
        fileTags: ["install"]
        files: [
            "images/folder_new.png",
        ]
    }
}

