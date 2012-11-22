
AutoTypes {
    imports: [ "import HelperWidgets 1.0", "import QtQuick 1.0", "import Bauhaus 1.0" ]

    Type {
       typeNames: ["int"]
       sourceFile: "IntEditorTemplate.qml"
    }
    Type {
        typeNames: ["real", "double", "qreal"]
        sourceFile: "RealEditorTemplate.qml"
    }
    Type {
        typeNames: ["string", "QString"]
        sourceFile: "StringEditorTemplate.qml"
    }
    Type {
        typeNames: ["QUrl", "url"]
        sourceFile: "UrlEditorTemplate.qml"
    }
    Type {
        typeNames: ["bool", "boolean"]
        sourceFile: "BooleanEditorTemplate.qml"
    }
    Type {
        typeNames: ["color", "QColor"]
        sourceFile: "ColorEditorTemplate.qml"
    }
}
