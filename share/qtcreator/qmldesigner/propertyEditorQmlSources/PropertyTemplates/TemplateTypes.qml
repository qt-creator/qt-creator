// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

AutoTypes {
    imports: [ "import HelperWidgets 2.0",
               "import QtQuick 2.15",
               "import QtQuick.Layouts 1.15",
               "import StudioTheme 1.0 as StudioTheme" ]

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

    Type {
        typeNames: ["Text"]
        sourceFile: "TextEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["font", "QFont"]
        sourceFile: "FontEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["Rectangle"]
        sourceFile: "RectangleEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["Image"]
        sourceFile: "ImageEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["TextureInput", "Texture"]
        sourceFile: "3DItemFilterComboBoxEditorTemplate.template"
        needsTypeArg: true
    }

    Type {
        typeNames: ["vector2d"]
        sourceFile: "Vector2dEditorTemplate.template"
    }

    Type {
        typeNames: ["vector3d"]
        sourceFile: "Vector3dEditorTemplate.template"
    }

    Type {
        typeNames: ["vector4d"]
        sourceFile: "Vector4dEditorTemplate.template"
    }
}
