import QtQuick 2.8

Item {
    id: buttonStyle
    width: 576
    height: 169

    Image {
        id: buttonStyleAsset
        x: 0
        y: 0
        source: "assets/ButtonStyle.png"
    }

    Image {
        id: buttonNormal
        x: 14
        y: 5
        source: "assets/buttonNormal.png"
    }

    Text {
        id: normalText
        x: 72
        y: 55
        color: "#BBBBBB"
        text: "Button Normal"
        font.letterSpacing: 0.594
        font.pixelSize: 24
    }

    Image {
        id: buttonPressed
        x: 290
        y: 5
        source: "assets/buttonPressed.png"
    }

    Text {
        id: pressedText
        x: 348
        y: 55
        color: "#E1E1E1"
        text: "Button Pressed"
        font.letterSpacing: 0.594
        font.pixelSize: 24
    }

    Text {
        id: annotation_text_to_b_219_17
        x: 78
        y: 137
        color: "#E1E1E1"
        text: "Annotation Text - To be skipped on import "
        font.letterSpacing: 0.594
        font.pixelSize: 24
    }
}

/*##^##
Designer {
    D{i:0;UUID:"2c9c0d7afb27ed7fc52953dffad06338"}D{i:1;UUID:"2c9c0d7afb27ed7fc52953dffad06338_asset"}
D{i:2;UUID:"fe9b73e310828dc3f213ba5ef14960b0"}D{i:3;UUID:"5c926d0aacc8788795645ff693c5211c"}
D{i:4;UUID:"261e8907a5da706273665c336dfec28a"}D{i:5;UUID:"7f46944edba773280017e4c8389c8ee0"}
D{i:6;UUID:"f4adf85e9cbce87a9e84a0aaa67d8a80"}
}
##^##*/

