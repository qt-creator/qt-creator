// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

AutoTypes {
    imports: [ "import HelperWidgets 2.0",
               "import QtQuick 2.15",
               "import QtQuick.Layouts 1.15",
               "import StudioTheme 1.0 as StudioTheme" ]

    Type {
       typeNames: ["int"]
       module: "QML"
       sourceFile: "IntEditorTemplate.template"
    }
    Type {
        typeNames: ["real", "double", "qreal"]
        module: "QML"
        sourceFile: "RealEditorTemplate.template"
    }
    Type {
        typeNames: ["string", "QString"]
        module: "QML"
        sourceFile: "StringEditorTemplate.template"
    }
    Type {
        typeNames: ["url", "QUrl"]
        module: "QML"
        sourceFile: "UrlEditorTemplate.template"
    }
    Type {
        typeNames: ["bool", "boolean"]
        module: "QML"
        sourceFile: "BooleanEditorTemplate.template"
    }

    Type {
        typeNames: ["color", "QColor"]
        module: "QtQuick"
        sourceFile: "ColorEditorTemplate.template"
    }

    Type {
        typeNames: ["Text"]
        module: "QtQuick"
        sourceFile: "TextEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["font", "QFont"]
        module: "QtQuick"
        sourceFile: "FontEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["Rectangle"]
        module: "QtQuick"
        sourceFile: "RectangleEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["Image"]
        module: "QtQuick"
        sourceFile: "ImageEditorTemplate.template"
        separateSection: true
    }

    Type {
        typeNames: ["TextureInput"]
        module: "QtQuick3D"
        sourceFile: "3DItemFilterComboBoxEditorTemplate.template"
        needsTypeArg: true
    }

    Type {
        typeNames: ["Texture"]
        module: "QtQuick3D"
        sourceFile: "3DItemFilterComboBoxEditorTemplate.template"
        needsTypeArg: true
    }

    Type {
        typeNames: ["vector2d"]
        module: "QtQuick3D"
        sourceFile: "Vector2dEditorTemplate.template"
    }

    Type {
        typeNames: ["vector3d"]
        module: "QtQuick3D"
        sourceFile: "Vector3dEditorTemplate.template"
    }

    Type {
        typeNames: ["vector4d"]
        module: "QtQuick3D"
        sourceFile: "Vector4dEditorTemplate.template"
    }
}
