import Qt 4.6

Rectangle {
    id: rect
    width: 200
    height: 200
    Text {
        id: text
        x: 66
        y: 93
        text: "Base State"
    }
    states: [
        State {
            name: "State1"
            PropertyChanges {
                target: rect
                color: "blue"
            }
            PropertyChanges {
                target: text
                text: "State1"
            }
        },
        State {
            name: "State2"
            PropertyChanges {
                target: rect
                color: "gray"
            }
            PropertyChanges {
                target: text
                text: "State2"
            }
        }
    ]

    Image {
        id: image1
        x: 41
        y: 46
        source: "images/qtcreator.png"
    }
}
