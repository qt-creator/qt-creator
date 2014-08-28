
AutoTypes {
    imports: [ "import HelperWidgets 2.0", "import QtQuick 2.1", "import QtQuick.Layouts 1.1" ]

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
        sourceFile: "StringEditorTemplate.template"
    }
}
