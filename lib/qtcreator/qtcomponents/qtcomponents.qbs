import qbs.base 1.0

Product {
    type: ["installed_content"]
    name: "QtComponents"

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/"
        fileTags: ["install"]
        files: [
            "Menu.qml",
            "ScrollBar.qml",
            "ContextMenu.qml",
            "TextArea.qml",
            "Switch.qml",
            "Tab.qml",
            "Slider.qml",
            "TabFrame.qml",
            "TextField.qml",
            "TabBar.qml",
            "MenuItem.qml",
            "Dial.qml",
            "ButtonRow.qml",
            "ToolBar.qml",
            "qmldir",
            "ProgressBar.qml",
            "RadioButton.qml",
            "TableColumn.qml",
            "GroupBox.qml",
            "Button.qml",
            "TableView.qml",
            "Frame.qml",
            "ToolButton.qml",
            "ScrollArea.qml",
            "SplitterRow.qml",
            "ChoiceList.qml",
            "CheckBox.qml",
            "SpinBox.qml",
        ]
    }

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/images"
        fileTags: ["install"]
        files: [
            "images/folder_new.png",
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
        qbs.installDir: "lib/qtcreator/qtcomponents/custom/behaviors"
        fileTags: ["install"]
        files: [
            "custom/behaviors/ButtonBehavior.qml",
            "custom/behaviors/ModalPopupBehavior.qml",
        ]
    }

    Group {
        qbs.installDir: "lib/qtcreator/qtcomponents/custom"
        fileTags: ["install"]
        files: [
            "custom/ButtonGroup.js",
            "custom/Slider.qml",
            "custom/TextField.qml",
            "custom/ButtonRow.qml",
            "custom/qmldir",
            "custom/BasicButton.qml",
            "custom/ProgressBar.qml",
            "custom/GroupBox.qml",
            "custom/Button.qml",
            "custom/ButtonColumn.qml",
            "custom/SplitterRow.qml",
            "custom/ChoiceList.qml",
            "custom/CheckBox.qml",
            "custom/SpinBox.qml",
        ]
    }
}

