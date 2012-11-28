
AutoTypes {
    imports: [ "import HelperWidgets 1.0", "import QtQuick 1.0", "import Bauhaus 1.0" ]

    Type {
       typeNames: ["int"]
       sourceFile: "IntEditorTemplate.template"
    }
    Type {
        typeNames: ["real", "double", "qreal"]
        sourceFile: "RealEditorTemplate.template"
    }
    Type {
        typeNames: ["string", "QString"]
        sourceFile: "StringEditorTemplate.template"
    }
    Type {
        typeNames: ["QUrl", "url"]
        sourceFile: "UrlEditorTemplate.template"
    }
    Type {
        typeNames: ["bool", "boolean"]
        sourceFile: "BooleanEditorTemplate.template"
    }
    Type {
        typeNames: ["color", "QColor"]
        sourceFile: "ColorEditorTemplate.template"
    }
}
