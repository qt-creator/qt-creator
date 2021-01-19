// nDiagnosticMessages=1
// InlineComponent.qml
import QtQuick 2.15

Item {
    component LabeledImage: Column {
        component NestedComp: Item {
        }
        property alias source: image.source
        property alias caption: text.text

        Image {
            id: image
            width: 50
            height: 50
        }
        Text {
            id: text
            font.bold: true
        }
    }

    Row {
        LabeledImage {
            id: before
            source: "before.png"
            caption: "Before"
        }
        LabeledImage {
            id: after
            source: "after.png"
            caption: "After"
        }
    }
    property LabeledImage selectedImage: before
}
