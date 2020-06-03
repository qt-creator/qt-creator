import QtQuick 2.10
import QtQuick.Templates 2.1 as T
import Home 1.0

T.Button {
    id: control
    width: 296
    height: 538

    font: Constants.font
    implicitWidth: Math.max(
                       background ? background.implicitWidth : 0,
                       contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        background ? background.implicitHeight : 0,
                        contentItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    background: normalBG
    contentItem: normalContent
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
            color: "#c1c1c1"
            text: "Normal button text"
            font.pixelSize: 30
            lineHeight: 44
            lineHeightMode: Text.FixedHeight
            wrapMode: Text.WordWrap
        }

        Text {
            id: normal_button_label
            x: 113
            y: 203
            color: "#c1c1c1"
            text: "Normal Button label"
            font.pixelSize: 36
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
            color: "#ffffff"
            text: "Focus button text"
            font.pixelSize: 30
            lineHeight: 44
            lineHeightMode: Text.FixedHeight
            wrapMode: Text.WordWrap
        }

        Text {
            id: focus_button_label
            x: 113
            y: 203
            color: "#ba544d"
            text: "Button label focus"
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

    Image {
        id: defaultBG
        x: 35
        y: 339
        source: "assets/defaultBG.png"
    }






    states: [
        State {
            name: "normal"
            when: !control.down && !control.focus

            PropertyChanges {
                target: focusedBG
                visible: false
            }
            PropertyChanges {
                target: focusedContent
                visible: false
            }
            PropertyChanges {
                target: pressedBG
                visible: false
            }
        },
        State {
            name: "press"
            when: control.down && control.focus
            PropertyChanges {
                target: control
                contentItem: focusedContent
            }

            PropertyChanges {
                target: normalBG
                visible: false
            }

            PropertyChanges {
                target: normalContent
                visible: false
            }

            PropertyChanges {
                target: pressedBG
                visible: true
            }

            PropertyChanges {
                target: focusedContent
                visible: true
            }

            PropertyChanges {
                target: control
                background: pressedBG
            }
        }
    ]
}
