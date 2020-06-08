import QtQuick 2.10

Item {
    id: info_screen
    width: 296
    height: 538

    Item {
        id: normalBG
        x: 0
        y: 0
        width: 296
        height: 538
        Image {
            id: normalBGAsset
            x: 0
            y: 0
            source: "assets/normalBG.png"
        }
    }

    Item {
        id: normalContent
        x: 0
        y: 42
        width: 296
        height: 428
        Image {
            id: normalContentAsset
            x: 115
            y: 40
            source: "assets/normalContent.png"
        }

        Text {
            id: normal_button_text
            x: 59
            y: 353
            width: 241
            height: 75
            color: "#C1C1C1"
            text: "Normal button text"
            font.pixelSize: 30
            lineHeightMode: Text.FixedHeight
            lineHeight: 44
            wrapMode: Text.WordWrap
        }

        Text {
            id: normal_button_label
            x: 113
            y: 203
            color: "#C1C1C1"
            text: "Normal Button label"
            font.pixelSize: 36
        }
    }

    Item {
        id: pressedBG
        x: 0
        y: 0
        width: 296
        height: 538
        Image {
            id: pressedBGAsset
            x: 0
            y: 0
            source: "assets/pressedBG.png"
        }

        Item {
            id: highlight_item_pressed_1
            x: 0
            y: 0
            width: 296
            height: 196
        }

        Item {
            id: highlight_item_pressed_2
            x: 39
            y: 0
            width: 197
            height: 99
        }

        Image {
            id: rectangle_pressedBG_1
            x: 0
            y: 0
            source: "assets/rectangle_pressedBG_1.png"
        }

        Image {
            id: highlight_img_pressed_1
            x: 0
            y: 0
            source: "assets/highlight_img_pressed_1.png"
        }

        Image {
            id: highlight_img_pressed_2
            x: 291
            y: 7
            source: "assets/highlight_img_pressed_2.png"
        }
    }

    Item {
        id: focusedBG
        x: 0
        y: 0
        width: 296
        height: 538
        Image {
            id: focusedBGAsset
            x: 0
            y: 0
            source: "assets/focusedBG.png"
        }

        Item {
            id: highlight_item_focused_1
            x: 0
            y: 0
            width: 296
            height: 196
        }

        Item {
            id: highlight_item_focused_2
            x: 39
            y: 0
            width: 197
            height: 99
        }

        Image {
            id: rectangle_focusedBG_1
            x: 0
            y: 0
            source: "assets/rectangle_focusedBG_1.png"
        }

        Image {
            id: highlight_img_focusedBG_1
            x: 0
            y: 0
            source: "assets/highlight_img_focusedBG_1.png"
        }

        Image {
            id: highlight_img_focused_1
            x: 291
            y: 7
            source: "assets/highlight_img_focused_1.png"
        }
    }

    Item {
        id: focusedContent
        x: 0
        y: 42
        width: 296
        height: 428
        Image {
            id: focusedContentAsset
            x: 115
            y: 40
            source: "assets/focusedContent.png"
        }

        Text {
            id: focus_button_text
            x: 59
            y: 353
            width: 241
            height: 75
            color: "#FFFFFF"
            text: "Focus button text"
            font.pixelSize: 30
            lineHeightMode: Text.FixedHeight
            lineHeight: 44
            wrapMode: Text.WordWrap
        }

        Text {
            id: focus_button_label
            x: 113
            y: 203
            color: "#BA544D"
            text: "Button label focus"
            font.pixelSize: 36
        }
    }

    Item {
        id: disabledBG
        x: 0
        y: 0
        width: 296
        height: 533
        Image {
            id: disabledBGAsset
            x: 0
            y: 0
            source: "assets/disabledBG.png"
        }
    }

    Item {
        id: disabledContent
        x: 0
        y: 0
        width: 296
        height: 538
        Image {
            id: disabledContentAsset
            x: 115
            y: 82
            source: "assets/disabledContent.png"
        }

        Text {
            id: disabled_button_text
            x: 59
            y: 395
            width: 241
            height: 75
            color: "#413E3C"
            text: "Disabled button text"
            font.pixelSize: 30
            lineHeightMode: Text.FixedHeight
            lineHeight: 44
            wrapMode: Text.WordWrap
        }

        Text {
            id: disabled_button_label
            x: 109
            y: 242
            color: "#413E3C"
            text: "Disabled button label"
            font.pixelSize: 40
        }
    }

    Image {
        id: defaultBG
        x: 35
        y: 339
        source: "assets/defaultBG.png"
    }
}
