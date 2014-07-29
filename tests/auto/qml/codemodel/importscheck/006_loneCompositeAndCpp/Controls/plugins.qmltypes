import QtQuick.tooling 1.1

// This file describes the plugin-supplied types contained in the library.
// It is used for QML tooling purposes only.
//
// This file was auto-generated with the command 'qmlplugindump -nonrelocatable QtQuick.Controls 1.1'.

Module {
    Component {
        name: "QQuickAbstractStyle"
        defaultProperty: "data"
        prototype: "QObject"
        exports: ["QtQuick.Controls.Private/AbstractStyle 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "padding"; type: "QQuickPadding"; isReadonly: true; isPointer: true }
        Property { name: "data"; type: "QObject"; isList: true; isReadonly: true }
    }
    Component {
        name: "QQuickAction"
        prototype: "QObject"
        exports: ["QtQuick.Controls/Action 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "text"; type: "string" }
        Property { name: "iconSource"; type: "QUrl" }
        Property { name: "iconName"; type: "string" }
        Property { name: "__icon"; type: "QVariant"; isReadonly: true }
        Property { name: "tooltip"; type: "string" }
        Property { name: "enabled"; type: "bool" }
        Property { name: "checkable"; type: "bool" }
        Property { name: "checked"; type: "bool" }
        Property { name: "exclusiveGroup"; type: "QQuickExclusiveGroup"; isPointer: true }
        Property { name: "shortcut"; type: "QVariant" }
        Signal { name: "triggered" }
        Signal {
            name: "toggled"
            Parameter { name: "checked"; type: "bool" }
        }
        Signal {
            name: "shortcutChanged"
            Parameter { name: "shortcut"; type: "QVariant" }
        }
        Signal { name: "iconChanged" }
        Signal {
            name: "tooltipChanged"
            Parameter { name: "arg"; type: "string" }
        }
        Method { name: "trigger" }
    }
    Component {
        name: "QQuickControlSettings"
        prototype: "QObject"
        exports: ["QtQuick.Controls.Private/Settings 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "style"; type: "QUrl"; isReadonly: true }
        Property { name: "styleName"; type: "string" }
        Property { name: "stylePath"; type: "string" }
        Property { name: "dpiScaleFactor"; type: "double"; isReadonly: true }
    }
    Component {
        name: "QQuickExclusiveGroup"
        defaultProperty: "__actions"
        prototype: "QObject"
        exports: ["QtQuick.Controls/ExclusiveGroup 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "current"; type: "QObject"; isPointer: true }
        Property { name: "__actions"; type: "QQuickAction"; isList: true; isReadonly: true }
        Method {
            name: "bindCheckable"
            Parameter { name: "o"; type: "QObject"; isPointer: true }
        }
        Method {
            name: "unbindCheckable"
            Parameter { name: "o"; type: "QObject"; isPointer: true }
        }
    }
    Component {
        name: "QQuickMenu"
        defaultProperty: "items"
        prototype: "QQuickMenuText"
        exports: ["QtQuick.Controls/MenuPrivate 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "title"; type: "string" }
        Property { name: "items"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "__selectedIndex"; type: "int" }
        Property { name: "__popupVisible"; type: "bool"; isReadonly: true }
        Property { name: "__contentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__minimumWidth"; type: "int" }
        Property { name: "__font"; type: "QFont" }
        Property { name: "__xOffset"; type: "double" }
        Property { name: "__yOffset"; type: "double" }
        Signal { name: "__menuClosed" }
        Signal { name: "popupVisibleChanged" }
        Method { name: "__closeMenu" }
        Method { name: "__dismissMenu" }
        Method { name: "popup" }
        Method {
            name: "addItem"
            type: "QQuickMenuItem*"
            Parameter { type: "string" }
        }
        Method {
            name: "insertItem"
            type: "QQuickMenuItem*"
            Parameter { type: "int" }
            Parameter { type: "string" }
        }
        Method { name: "addSeparator" }
        Method {
            name: "insertSeparator"
            Parameter { type: "int" }
        }
        Method {
            name: "insertItem"
            Parameter { type: "int" }
            Parameter { type: "QQuickMenuBase"; isPointer: true }
        }
        Method {
            name: "removeItem"
            Parameter { type: "QQuickMenuBase"; isPointer: true }
        }
        Method { name: "clear" }
        Method {
            name: "__popup"
            Parameter { name: "x"; type: "double" }
            Parameter { name: "y"; type: "double" }
            Parameter { name: "atActionIndex"; type: "int" }
        }
        Method {
            name: "__popup"
            Parameter { name: "x"; type: "double" }
            Parameter { name: "y"; type: "double" }
        }
    }
    Component {
        name: "QQuickMenuBar"
        defaultProperty: "menus"
        prototype: "QObject"
        exports: ["QtQuick.Controls/MenuBarPrivate 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "menus"; type: "QQuickMenu"; isList: true; isReadonly: true }
        Property { name: "__contentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__parentWindow"; type: "QQuickWindow"; isPointer: true }
        Property { name: "__isNative"; type: "bool"; isReadonly: true }
        Signal { name: "contentItemChanged" }
    }
    Component {
        name: "QQuickMenuBase"
        prototype: "QObject"
        exports: ["QtQuick.Controls/MenuBase 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "visible"; type: "bool" }
        Property { name: "type"; type: "QQuickMenuItemType::MenuItemType"; isReadonly: true }
        Property { name: "__parentMenu"; type: "QQuickMenu"; isReadonly: true; isPointer: true }
        Property { name: "__isNative"; type: "bool"; isReadonly: true }
        Property { name: "__visualItem"; type: "QQuickItem"; isPointer: true }
    }
    Component {
        name: "QQuickMenuItem"
        prototype: "QQuickMenuText"
        exports: ["QtQuick.Controls/MenuItem 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "text"; type: "string" }
        Property { name: "checkable"; type: "bool" }
        Property { name: "checked"; type: "bool" }
        Property { name: "exclusiveGroup"; type: "QQuickExclusiveGroup"; isPointer: true }
        Property { name: "shortcut"; type: "QVariant" }
        Property { name: "action"; type: "QQuickAction"; isPointer: true }
        Signal { name: "triggered" }
        Signal {
            name: "toggled"
            Parameter { name: "checked"; type: "bool" }
        }
        Method { name: "trigger" }
    }
    Component {
        name: "QQuickMenuItemType"
        exports: ["QtQuick.Controls/MenuItemType 1.0"]
        exportMetaObjectRevisions: [0]
        Enum {
            name: "MenuItemType"
            values: {
                "Separator": 0,
                "Item": 1,
                "Menu": 2
            }
        }
    }
    Component {
        name: "QQuickMenuSeparator"
        prototype: "QQuickMenuBase"
        exports: ["QtQuick.Controls/MenuSeparator 1.0"]
        exportMetaObjectRevisions: [0]
    }
    Component {
        name: "QQuickMenuText"
        prototype: "QQuickMenuBase"
        Property { name: "enabled"; type: "bool" }
        Property { name: "iconSource"; type: "QUrl" }
        Property { name: "iconName"; type: "string" }
        Property { name: "__icon"; type: "QVariant"; isReadonly: true }
        Signal { name: "__textChanged" }
    }
    Component {
        name: "QQuickPadding"
        prototype: "QObject"
        Property { name: "left"; type: "int" }
        Property { name: "top"; type: "int" }
        Property { name: "right"; type: "int" }
        Property { name: "bottom"; type: "int" }
        Method {
            name: "setLeft"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setTop"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setRight"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setBottom"
            Parameter { name: "arg"; type: "int" }
        }
    }
    Component {
        name: "QQuickRangeModel"
        prototype: "QObject"
        exports: ["QtQuick.Controls.Private/RangeModel 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "value"; type: "double" }
        Property { name: "minimumValue"; type: "double" }
        Property { name: "maximumValue"; type: "double" }
        Property { name: "stepSize"; type: "double" }
        Property { name: "position"; type: "double" }
        Property { name: "positionAtMinimum"; type: "double" }
        Property { name: "positionAtMaximum"; type: "double" }
        Property { name: "inverted"; type: "bool" }
        Signal {
            name: "valueChanged"
            Parameter { name: "value"; type: "double" }
        }
        Signal {
            name: "positionChanged"
            Parameter { name: "position"; type: "double" }
        }
        Signal {
            name: "stepSizeChanged"
            Parameter { name: "stepSize"; type: "double" }
        }
        Signal {
            name: "invertedChanged"
            Parameter { name: "inverted"; type: "bool" }
        }
        Signal {
            name: "minimumChanged"
            Parameter { name: "min"; type: "double" }
        }
        Signal {
            name: "maximumChanged"
            Parameter { name: "max"; type: "double" }
        }
        Signal {
            name: "positionAtMinimumChanged"
            Parameter { name: "min"; type: "double" }
        }
        Signal {
            name: "positionAtMaximumChanged"
            Parameter { name: "max"; type: "double" }
        }
        Method { name: "toMinimum" }
        Method { name: "toMaximum" }
        Method {
            name: "setValue"
            Parameter { name: "value"; type: "double" }
        }
        Method {
            name: "setPosition"
            Parameter { name: "position"; type: "double" }
        }
        Method {
            name: "valueForPosition"
            type: "double"
            Parameter { name: "position"; type: "double" }
        }
        Method {
            name: "positionForValue"
            type: "double"
            Parameter { name: "value"; type: "double" }
        }
    }
    Component {
        name: "QQuickSpinBoxValidator"
        prototype: "QValidator"
        exports: ["QtQuick.Controls.Private/SpinBoxValidator 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "text"; type: "string"; isReadonly: true }
        Property { name: "value"; type: "double" }
        Property { name: "minimumValue"; type: "double" }
        Property { name: "maximumValue"; type: "double" }
        Property { name: "decimals"; type: "int" }
        Property { name: "stepSize"; type: "double" }
        Property { name: "prefix"; type: "string" }
        Property { name: "suffix"; type: "string" }
        Method { name: "increment" }
        Method { name: "decrement" }
    }
    Component {
        name: "QQuickStack"
        prototype: "QObject"
        exports: ["QtQuick.Controls/Stack 1.0"]
        exportMetaObjectRevisions: [0]
        Enum {
            name: "Status"
            values: {
                "Inactive": 0,
                "Deactivating": 1,
                "Activating": 2,
                "Active": 3
            }
        }
        Property { name: "index"; type: "int"; isReadonly: true }
        Property { name: "__index"; type: "int" }
        Property { name: "status"; type: "Status"; isReadonly: true }
        Property { name: "__status"; type: "Status" }
        Property { name: "view"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Property { name: "__view"; type: "QQuickItem"; isPointer: true }
    }
    Component {
        name: "QQuickStyleItem"
        defaultProperty: "data"
        prototype: "QQuickItem"
        exports: ["QtQuick.Controls.Private/StyleItem 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "sunken"; type: "bool" }
        Property { name: "raised"; type: "bool" }
        Property { name: "active"; type: "bool" }
        Property { name: "selected"; type: "bool" }
        Property { name: "hasFocus"; type: "bool" }
        Property { name: "on"; type: "bool" }
        Property { name: "hover"; type: "bool" }
        Property { name: "horizontal"; type: "bool" }
        Property { name: "transient"; type: "bool" }
        Property { name: "elementType"; type: "string" }
        Property { name: "text"; type: "string" }
        Property { name: "activeControl"; type: "string" }
        Property { name: "style"; type: "string"; isReadonly: true }
        Property { name: "hints"; type: "QVariantMap" }
        Property { name: "properties"; type: "QVariantMap" }
        Property { name: "font"; type: "QFont"; isReadonly: true }
        Property { name: "minimum"; type: "int" }
        Property { name: "maximum"; type: "int" }
        Property { name: "value"; type: "int" }
        Property { name: "step"; type: "int" }
        Property { name: "paintMargins"; type: "int" }
        Property { name: "contentWidth"; type: "int" }
        Property { name: "contentHeight"; type: "int" }
        Signal { name: "infoChanged" }
        Signal { name: "hintChanged" }
        Signal {
            name: "contentWidthChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "contentHeightChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "pixelMetric"
            type: "int"
            Parameter { type: "string" }
        }
        Method {
            name: "styleHint"
            type: "QVariant"
            Parameter { type: "string" }
        }
        Method { name: "updateSizeHint" }
        Method { name: "updateRect" }
        Method { name: "updateItem" }
        Method {
            name: "hitTest"
            type: "string"
            Parameter { name: "x"; type: "int" }
            Parameter { name: "y"; type: "int" }
        }
        Method {
            name: "subControlRect"
            type: "QRectF"
            Parameter { name: "subcontrolString"; type: "string" }
        }
        Method {
            name: "elidedText"
            type: "string"
            Parameter { name: "text"; type: "string" }
            Parameter { name: "elideMode"; type: "int" }
            Parameter { name: "width"; type: "int" }
        }
        Method {
            name: "hasThemeIcon"
            type: "bool"
            Parameter { type: "string" }
        }
        Method {
            name: "textWidth"
            type: "double"
            Parameter { type: "string" }
        }
        Method {
            name: "textHeight"
            type: "double"
            Parameter { type: "string" }
        }
    }
    Component {
        name: "QQuickTooltip"
        prototype: "QObject"
        exports: ["QtQuick.Controls.Private/Tooltip 1.0"]
        exportMetaObjectRevisions: [0]
        Method {
            name: "showText"
            Parameter { name: "item"; type: "QQuickItem"; isPointer: true }
            Parameter { name: "pos"; type: "QPointF" }
            Parameter { name: "text"; type: "string" }
        }
        Method { name: "hideText" }
    }
    Component {
        name: "QQuickWheelArea"
        defaultProperty: "data"
        prototype: "QQuickItem"
        exports: ["QtQuick.Controls.Private/WheelArea 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "verticalDelta"; type: "double" }
        Property { name: "horizontalDelta"; type: "double" }
        Property { name: "horizontalMinimumValue"; type: "double" }
        Property { name: "horizontalMaximumValue"; type: "double" }
        Property { name: "verticalMinimumValue"; type: "double" }
        Property { name: "verticalMaximumValue"; type: "double" }
        Property { name: "horizontalValue"; type: "double" }
        Property { name: "verticalValue"; type: "double" }
        Property { name: "scrollSpeed"; type: "double" }
        Property { name: "active"; type: "bool" }
        Signal { name: "verticalWheelMoved" }
        Signal { name: "horizontalWheelMoved" }
    }
    Component {
        name: "QQuickWindow"
        defaultProperty: "data"
        prototype: "QWindow"
        exports: ["QtQuick.Window/Window 2.0", "QtQuick.Window/Window 2.1"]
        exportMetaObjectRevisions: [0, 1]
        Property { name: "data"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "color"; type: "QColor" }
        Property { name: "contentItem"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Property {
            name: "activeFocusItem"
            revision: 1
            type: "QQuickItem"
            isReadonly: true
            isPointer: true
        }
        Signal { name: "frameSwapped" }
        Signal { name: "sceneGraphInitialized" }
        Signal { name: "sceneGraphInvalidated" }
        Signal { name: "beforeSynchronizing" }
        Signal { name: "beforeRendering" }
        Signal { name: "afterRendering" }
        Signal {
            name: "closing"
            revision: 1
            Parameter { name: "close"; type: "QQuickCloseEvent"; isPointer: true }
        }
        Signal {
            name: "colorChanged"
            Parameter { type: "QColor" }
        }
        Signal { name: "activeFocusItemChanged"; revision: 1 }
        Method { name: "update" }
        Method { name: "releaseResources" }
    }
    Component {
        name: "QWindow"
        prototype: "QObject"
        Enum {
            name: "Visibility"
            values: {
                "Hidden": 0,
                "AutomaticVisibility": 1,
                "Windowed": 2,
                "Minimized": 3,
                "Maximized": 4,
                "FullScreen": 5
            }
        }
        Property { name: "title"; type: "string" }
        Property { name: "modality"; type: "Qt::WindowModality" }
        Property { name: "flags"; type: "Qt::WindowFlags" }
        Property { name: "x"; type: "int" }
        Property { name: "y"; type: "int" }
        Property { name: "width"; type: "int" }
        Property { name: "height"; type: "int" }
        Property { name: "minimumWidth"; revision: 1; type: "int" }
        Property { name: "minimumHeight"; revision: 1; type: "int" }
        Property { name: "maximumWidth"; revision: 1; type: "int" }
        Property { name: "maximumHeight"; revision: 1; type: "int" }
        Property { name: "visible"; type: "bool" }
        Property { name: "active"; revision: 1; type: "bool"; isReadonly: true }
        Property { name: "visibility"; revision: 1; type: "Visibility" }
        Property { name: "contentOrientation"; revision: 1; type: "Qt::ScreenOrientation" }
        Property { name: "opacity"; revision: 1; type: "double" }
        Signal {
            name: "screenChanged"
            Parameter { name: "screen"; type: "QScreen"; isPointer: true }
        }
        Signal {
            name: "modalityChanged"
            Parameter { name: "modality"; type: "Qt::WindowModality" }
        }
        Signal {
            name: "windowStateChanged"
            Parameter { name: "windowState"; type: "Qt::WindowState" }
        }
        Signal {
            name: "xChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "yChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "widthChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "heightChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "minimumWidthChanged"
            revision: 1
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "minimumHeightChanged"
            revision: 1
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "maximumWidthChanged"
            revision: 1
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "maximumHeightChanged"
            revision: 1
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "visibleChanged"
            Parameter { name: "arg"; type: "bool" }
        }
        Signal {
            name: "visibilityChanged"
            revision: 1
            Parameter { name: "visibility"; type: "QWindow::Visibility" }
        }
        Signal { name: "activeChanged"; revision: 1 }
        Signal {
            name: "contentOrientationChanged"
            revision: 1
            Parameter { name: "orientation"; type: "Qt::ScreenOrientation" }
        }
        Signal {
            name: "focusObjectChanged"
            Parameter { name: "object"; type: "QObject"; isPointer: true }
        }
        Signal {
            name: "opacityChanged"
            revision: 1
            Parameter { name: "opacity"; type: "double" }
        }
        Method { name: "requestActivate"; revision: 1 }
        Method {
            name: "setVisible"
            Parameter { name: "visible"; type: "bool" }
        }
        Method { name: "show" }
        Method { name: "hide" }
        Method { name: "showMinimized" }
        Method { name: "showMaximized" }
        Method { name: "showFullScreen" }
        Method { name: "showNormal" }
        Method { name: "close"; type: "bool" }
        Method { name: "raise" }
        Method { name: "lower" }
        Method {
            name: "setTitle"
            Parameter { type: "string" }
        }
        Method {
            name: "setX"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setY"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setWidth"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setHeight"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setMinimumWidth"
            revision: 1
            Parameter { name: "w"; type: "int" }
        }
        Method {
            name: "setMinimumHeight"
            revision: 1
            Parameter { name: "h"; type: "int" }
        }
        Method {
            name: "setMaximumWidth"
            revision: 1
            Parameter { name: "w"; type: "int" }
        }
        Method {
            name: "setMaximumHeight"
            revision: 1
            Parameter { name: "h"; type: "int" }
        }
        Method {
            name: "alert"
            Parameter { name: "msec"; type: "int" }
        }
    }
    Component {
        prototype: "QQuickWindow"
        name: "QtQuick.Controls/ApplicationWindow"
        exports: ["QtQuick.Controls/ApplicationWindow 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "menuBar"; type: "MenuBar_QMLTYPE_1"; isPointer: true }
        Property { name: "toolBar"; type: "QQuickItem"; isPointer: true }
        Property { name: "statusBar"; type: "QQuickItem"; isPointer: true }
        Property { name: "data"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "data"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "color"; type: "QColor" }
        Property { name: "contentItem"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Property {
            name: "activeFocusItem"
            revision: 1
            type: "QQuickItem"
            isReadonly: true
            isPointer: true
        }
        Signal { name: "frameSwapped" }
        Signal { name: "sceneGraphInitialized" }
        Signal { name: "sceneGraphInvalidated" }
        Signal { name: "beforeSynchronizing" }
        Signal { name: "beforeRendering" }
        Signal { name: "afterRendering" }
        Signal {
            name: "closing"
            revision: 1
            Parameter { name: "close"; type: "QQuickCloseEvent"; isPointer: true }
        }
        Signal {
            name: "colorChanged"
            Parameter { type: "QColor" }
        }
        Signal { name: "activeFocusItemChanged"; revision: 1 }
        Method { name: "update" }
        Method { name: "forcePolish" }
        Method { name: "releaseResources" }
        Property { name: "title"; type: "string" }
        Property { name: "modality"; type: "Qt::WindowModality" }
        Property { name: "flags"; type: "Qt::WindowFlags" }
        Property { name: "x"; type: "int" }
        Property { name: "y"; type: "int" }
        Property { name: "width"; type: "int" }
        Property { name: "height"; type: "int" }
        Property { name: "minimumWidth"; type: "int" }
        Property { name: "minimumHeight"; type: "int" }
        Property { name: "maximumWidth"; type: "int" }
        Property { name: "maximumHeight"; type: "int" }
        Property { name: "visible"; type: "bool" }
        Property { name: "active"; revision: 1; type: "bool"; isReadonly: true }
        Property { name: "visibility"; revision: 1; type: "Visibility" }
        Property { name: "contentOrientation"; type: "Qt::ScreenOrientation" }
        Property { name: "opacity"; revision: 1; type: "double" }
        Signal {
            name: "screenChanged"
            Parameter { name: "screen"; type: "QScreen"; isPointer: true }
        }
        Signal {
            name: "modalityChanged"
            Parameter { name: "modality"; type: "Qt::WindowModality" }
        }
        Signal {
            name: "windowStateChanged"
            Parameter { name: "windowState"; type: "Qt::WindowState" }
        }
        Signal {
            name: "xChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "yChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "widthChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "heightChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "minimumWidthChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "minimumHeightChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "maximumWidthChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "maximumHeightChanged"
            Parameter { name: "arg"; type: "int" }
        }
        Signal {
            name: "visibleChanged"
            Parameter { name: "arg"; type: "bool" }
        }
        Signal {
            name: "visibilityChanged"
            revision: 1
            Parameter { name: "visibility"; type: "QWindow::Visibility" }
        }
        Signal { name: "activeChanged"; revision: 1 }
        Signal {
            name: "contentOrientationChanged"
            Parameter { name: "orientation"; type: "Qt::ScreenOrientation" }
        }
        Signal {
            name: "focusObjectChanged"
            Parameter { name: "object"; type: "QObject"; isPointer: true }
        }
        Signal {
            name: "opacityChanged"
            revision: 1
            Parameter { name: "opacity"; type: "double" }
        }
        Method { name: "requestActivate"; revision: 1 }
        Method {
            name: "setVisible"
            Parameter { name: "visible"; type: "bool" }
        }
        Method { name: "show" }
        Method { name: "hide" }
        Method { name: "showMinimized" }
        Method { name: "showMaximized" }
        Method { name: "showFullScreen" }
        Method { name: "showNormal" }
        Method { name: "close"; type: "bool" }
        Method { name: "raise" }
        Method { name: "lower" }
        Method {
            name: "setTitle"
            Parameter { type: "string" }
        }
        Method {
            name: "setX"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setY"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setWidth"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setHeight"
            Parameter { name: "arg"; type: "int" }
        }
        Method {
            name: "setMinimumWidth"
            Parameter { name: "w"; type: "int" }
        }
        Method {
            name: "setMinimumHeight"
            Parameter { name: "h"; type: "int" }
        }
        Method {
            name: "setMaximumWidth"
            Parameter { name: "w"; type: "int" }
        }
        Method {
            name: "setMaximumHeight"
            Parameter { name: "h"; type: "int" }
        }
        Method {
            name: "alert"
            revision: 1
            Parameter { name: "msec"; type: "int" }
        }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/BusyIndicator"
        exports: ["QtQuick.Controls/BusyIndicator 1.1"]
        exportMetaObjectRevisions: [1]
        defaultProperty: "data"
        Property { name: "running"; type: "bool" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/Button"
        exports: ["QtQuick.Controls/Button 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "checkable"; type: "bool" }
        Property { name: "checked"; type: "bool" }
        Property { name: "exclusiveGroup"; type: "QQuickExclusiveGroup"; isPointer: true }
        Property { name: "action"; type: "QQuickAction"; isPointer: true }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "text"; type: "string" }
        Property { name: "tooltip"; type: "string" }
        Property { name: "iconSource"; type: "QUrl" }
        Property { name: "iconName"; type: "string" }
        Property { name: "__textColor"; type: "QColor" }
        Property { name: "__position"; type: "string" }
        Property { name: "__iconOverriden"; type: "bool"; isReadonly: true }
        Property { name: "__action"; type: "QQuickAction"; isPointer: true }
        Property { name: "__iconAction"; type: "QQuickAction"; isReadonly: true; isPointer: true }
        Property { name: "__effectivePressed"; type: "bool" }
        Property { name: "__behavior"; type: "QVariant" }
        Property { name: "pressed"; type: "bool"; isReadonly: true }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Signal { name: "clicked" }
        Method { name: "accessiblePressAction"; type: "QVariant" }
        Property { name: "isDefault"; type: "bool" }
        Property { name: "menu"; type: "Menu_QMLTYPE_16"; isPointer: true }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/CheckBox"
        exports: ["QtQuick.Controls/CheckBox 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "checked"; type: "bool" }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "exclusiveGroup"; type: "QQuickExclusiveGroup"; isPointer: true }
        Property { name: "text"; type: "string" }
        Property { name: "__cycleStatesHandler"; type: "QVariant" }
        Property { name: "pressed"; type: "bool" }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Signal { name: "clicked" }
        Property { name: "checkedState"; type: "int" }
        Property { name: "partiallyCheckedEnabled"; type: "bool" }
        Property { name: "__ignoreChecked"; type: "bool" }
        Method { name: "__cycleCheckBoxStates"; type: "QVariant" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/ComboBox"
        exports: ["QtQuick.Controls/ComboBox 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "textRole"; type: "string" }
        Property { name: "editable"; type: "bool" }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "pressed"; type: "bool"; isReadonly: true }
        Property { name: "__popup"; type: "QVariant" }
        Property { name: "model"; type: "QVariant" }
        Property { name: "currentIndex"; type: "int" }
        Property { name: "currentText"; type: "string"; isReadonly: true }
        Property { name: "editText"; type: "string" }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Property { name: "count"; type: "int"; isReadonly: true }
        Property { name: "validator"; type: "QValidator"; isPointer: true }
        Property { name: "acceptableInput"; type: "bool"; isReadonly: true }
        Signal { name: "accepted" }
        Signal {
            name: "activated"
            Parameter { name: "index"; type: "int" }
        }
        Method {
            name: "textAt"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
        }
        Method {
            name: "find"
            type: "QVariant"
            Parameter { name: "text"; type: "QVariant" }
        }
        Method { name: "selectAll"; type: "QVariant" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/GroupBox"
        exports: ["QtQuick.Controls/GroupBox 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "__content"
        Property { name: "title"; type: "string" }
        Property { name: "flat"; type: "bool" }
        Property { name: "checkable"; type: "bool" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "checked"; type: "bool" }
        Property { name: "__content"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "contentItem"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Property { name: "__checkbox"; type: "CheckBox_QMLTYPE_25"; isReadonly: true; isPointer: true }
        Property { name: "__style"; type: "QObject"; isReadonly: true; isPointer: true }
    }
    Component {
        name: "QQuickText"
        defaultProperty: "data"
        prototype: "QQuickImplicitSizeItem"
        exports: ["QtQuick/Text 2.0"]
        exportMetaObjectRevisions: [0]
        Enum {
            name: "HAlignment"
            values: {
                "AlignLeft": 1,
                "AlignRight": 2,
                "AlignHCenter": 4,
                "AlignJustify": 8
            }
        }
        Enum {
            name: "VAlignment"
            values: {
                "AlignTop": 32,
                "AlignBottom": 64,
                "AlignVCenter": 128
            }
        }
        Enum {
            name: "TextStyle"
            values: {
                "Normal": 0,
                "Outline": 1,
                "Raised": 2,
                "Sunken": 3
            }
        }
        Enum {
            name: "TextFormat"
            values: {
                "PlainText": 0,
                "RichText": 1,
                "AutoText": 2,
                "StyledText": 4
            }
        }
        Enum {
            name: "TextElideMode"
            values: {
                "ElideLeft": 0,
                "ElideRight": 1,
                "ElideMiddle": 2,
                "ElideNone": 3
            }
        }
        Enum {
            name: "WrapMode"
            values: {
                "NoWrap": 0,
                "WordWrap": 1,
                "WrapAnywhere": 3,
                "WrapAtWordBoundaryOrAnywhere": 4,
                "Wrap": 4
            }
        }
        Enum {
            name: "RenderType"
            values: {
                "QtRendering": 0,
                "NativeRendering": 1
            }
        }
        Enum {
            name: "LineHeightMode"
            values: {
                "ProportionalHeight": 0,
                "FixedHeight": 1
            }
        }
        Enum {
            name: "FontSizeMode"
            values: {
                "FixedSize": 0,
                "HorizontalFit": 1,
                "VerticalFit": 2,
                "Fit": 3
            }
        }
        Property { name: "text"; type: "string" }
        Property { name: "font"; type: "QFont" }
        Property { name: "color"; type: "QColor" }
        Property { name: "linkColor"; type: "QColor" }
        Property { name: "style"; type: "TextStyle" }
        Property { name: "styleColor"; type: "QColor" }
        Property { name: "horizontalAlignment"; type: "HAlignment" }
        Property { name: "effectiveHorizontalAlignment"; type: "HAlignment"; isReadonly: true }
        Property { name: "verticalAlignment"; type: "VAlignment" }
        Property { name: "wrapMode"; type: "WrapMode" }
        Property { name: "lineCount"; type: "int"; isReadonly: true }
        Property { name: "truncated"; type: "bool"; isReadonly: true }
        Property { name: "maximumLineCount"; type: "int" }
        Property { name: "textFormat"; type: "TextFormat" }
        Property { name: "elide"; type: "TextElideMode" }
        Property { name: "contentWidth"; type: "double"; isReadonly: true }
        Property { name: "contentHeight"; type: "double"; isReadonly: true }
        Property { name: "paintedWidth"; type: "double"; isReadonly: true }
        Property { name: "paintedHeight"; type: "double"; isReadonly: true }
        Property { name: "lineHeight"; type: "double" }
        Property { name: "lineHeightMode"; type: "LineHeightMode" }
        Property { name: "baseUrl"; type: "QUrl" }
        Property { name: "minimumPixelSize"; type: "int" }
        Property { name: "minimumPointSize"; type: "int" }
        Property { name: "fontSizeMode"; type: "FontSizeMode" }
        Property { name: "renderType"; type: "RenderType" }
        Signal {
            name: "textChanged"
            Parameter { name: "text"; type: "string" }
        }
        Signal {
            name: "linkActivated"
            Parameter { name: "link"; type: "string" }
        }
        Signal {
            name: "fontChanged"
            Parameter { name: "font"; type: "QFont" }
        }
        Signal {
            name: "styleChanged"
            Parameter { name: "style"; type: "TextStyle" }
        }
        Signal {
            name: "horizontalAlignmentChanged"
            Parameter { name: "alignment"; type: "HAlignment" }
        }
        Signal {
            name: "verticalAlignmentChanged"
            Parameter { name: "alignment"; type: "VAlignment" }
        }
        Signal {
            name: "textFormatChanged"
            Parameter { name: "textFormat"; type: "TextFormat" }
        }
        Signal {
            name: "elideModeChanged"
            Parameter { name: "mode"; type: "TextElideMode" }
        }
        Signal { name: "contentSizeChanged" }
        Signal {
            name: "lineHeightChanged"
            Parameter { name: "lineHeight"; type: "double" }
        }
        Signal {
            name: "lineHeightModeChanged"
            Parameter { name: "mode"; type: "LineHeightMode" }
        }
        Signal {
            name: "lineLaidOut"
            Parameter { name: "line"; type: "QQuickTextLine"; isPointer: true }
        }
        Method { name: "doLayout" }
    }

    Component {
        prototype: "QQuickText"
        name: "QtQuick.Controls/Label"
        exports: ["QtQuick.Controls/Label 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
    }
    Component {
        prototype: "QQuickMenu"
        name: "QtQuick.Controls/Menu"
        exports: ["QtQuick.Controls/Menu 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "items"
        Property { name: "__selfComponent"; type: "QQmlComponent"; isPointer: true }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__currentIndex"; type: "int" }
        Property { name: "__menuComponent"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__menuBar"; type: "QVariant" }
        Method {
            name: "addMenu"
            type: "QVariant"
            Parameter { name: "title"; type: "QVariant" }
        }
        Method {
            name: "insertMenu"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
            Parameter { name: "title"; type: "QVariant" }
        }
        Property { name: "title"; type: "string" }
        Property { name: "items"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "__selectedIndex"; type: "int" }
        Property { name: "__popupVisible"; type: "bool"; isReadonly: true }
        Property { name: "__contentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__minimumWidth"; type: "int" }
        Property { name: "__font"; type: "QFont" }
        Property { name: "__xOffset"; type: "double" }
        Property { name: "__yOffset"; type: "double" }
        Signal { name: "__menuClosed" }
        Signal { name: "popupVisibleChanged" }
        Method { name: "__closeMenu" }
        Method { name: "__dismissMenu" }
        Method { name: "popup" }
        Method {
            name: "addItem"
            type: "QQuickMenuItem*"
            Parameter { type: "string" }
        }
        Method {
            name: "insertItem"
            type: "QQuickMenuItem*"
            Parameter { type: "int" }
            Parameter { type: "string" }
        }
        Method { name: "addSeparator" }
        Method {
            name: "insertSeparator"
            Parameter { type: "int" }
        }
        Method {
            name: "insertItem"
            Parameter { type: "int" }
            Parameter { type: "QQuickMenuBase"; isPointer: true }
        }
        Method {
            name: "removeItem"
            Parameter { type: "QQuickMenuBase"; isPointer: true }
        }
        Method { name: "clear" }
        Method {
            name: "__popup"
            Parameter { name: "x"; type: "double" }
            Parameter { name: "y"; type: "double" }
            Parameter { name: "atActionIndex"; type: "int" }
        }
        Method {
            name: "__popup"
            Parameter { name: "x"; type: "double" }
            Parameter { name: "y"; type: "double" }
        }
        Property { name: "visible"; type: "bool" }
        Property { name: "type"; type: "QQuickMenuItemType::MenuItemType"; isReadonly: true }
        Property { name: "__parentMenu"; type: "QQuickMenu"; isReadonly: true; isPointer: true }
        Property { name: "__isNative"; type: "bool"; isReadonly: true }
        Property { name: "__visualItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "enabled"; type: "bool" }
        Property { name: "iconSource"; type: "QUrl" }
        Property { name: "iconName"; type: "string" }
        Property { name: "__icon"; type: "QVariant"; isReadonly: true }
        Signal { name: "__textChanged" }
    }
    Component {
        prototype: "QQuickMenuBar"
        name: "QtQuick.Controls/MenuBar"
        exports: ["QtQuick.Controls/MenuBar 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "menus"
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__menuBarComponent"; type: "QQmlComponent"; isPointer: true }
        Property { name: "menus"; type: "QQuickMenu"; isList: true; isReadonly: true }
        Property { name: "__contentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__parentWindow"; type: "QQuickWindow"; isPointer: true }
        Property { name: "__isNative"; type: "bool"; isReadonly: true }
        Signal { name: "contentItemChanged" }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/ProgressBar"
        exports: ["QtQuick.Controls/ProgressBar 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
        Property { name: "value"; type: "double" }
        Property { name: "minimumValue"; type: "double" }
        Property { name: "maximumValue"; type: "double" }
        Property { name: "indeterminate"; type: "bool" }
        Property { name: "orientation"; type: "int" }
        Property { name: "__initialized"; type: "bool" }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Method {
            name: "setValue"
            type: "QVariant"
            Parameter { name: "v"; type: "QVariant" }
        }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/RadioButton"
        exports: ["QtQuick.Controls/RadioButton 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "checked"; type: "bool" }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "exclusiveGroup"; type: "QQuickExclusiveGroup"; isPointer: true }
        Property { name: "text"; type: "string" }
        Property { name: "__cycleStatesHandler"; type: "QVariant" }
        Property { name: "pressed"; type: "bool" }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Signal { name: "clicked" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/ScrollView"
        exports: ["QtQuick.Controls/ScrollView 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "contentItem"
        Property { name: "frameVisible"; type: "bool" }
        Property { name: "highlightOnFocus"; type: "bool" }
        Property { name: "contentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__scroller"; type: "QQuickItem"; isPointer: true }
        Property { name: "__scrollBarTopMargin"; type: "int" }
        Property { name: "__viewTopMargin"; type: "int" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "Style_QMLTYPE_0"; isPointer: true }
        Property { name: "viewport"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Property { name: "flickableItem"; type: "QQuickFlickable"; isReadonly: true; isPointer: true }
        Property {
            name: "__horizontalScrollBar"
            type: "ScrollBar_QMLTYPE_51"
            isReadonly: true
            isPointer: true
        }
        Property {
            name: "__verticalScrollBar"
            type: "ScrollBar_QMLTYPE_51"
            isReadonly: true
            isPointer: true
        }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/Slider"
        exports: ["QtQuick.Controls/Slider 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
        Property { name: "orientation"; type: "int" }
        Property { name: "updateValueWhileDragging"; type: "bool" }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "tickmarksEnabled"; type: "bool" }
        Property { name: "__horizontal"; type: "bool" }
        Property { name: "__handlePos"; type: "double" }
        Property { name: "minimumValue"; type: "double" }
        Property { name: "maximumValue"; type: "double" }
        Property { name: "pressed"; type: "bool"; isReadonly: true }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Property { name: "stepSize"; type: "double" }
        Property { name: "value"; type: "double" }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/SpinBox"
        exports: ["QtQuick.Controls/SpinBox 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "value"; type: "double" }
        Property { name: "minimumValue"; type: "double" }
        Property { name: "maximumValue"; type: "double" }
        Property { name: "stepSize"; type: "double" }
        Property { name: "suffix"; type: "string" }
        Property { name: "prefix"; type: "string" }
        Property { name: "decimals"; type: "int" }
        Property { name: "font"; type: "QFont" }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Property { name: "__text"; type: "string" }
        Method { name: "__increment"; type: "QVariant" }
        Method { name: "__decrement"; type: "QVariant" }
    }
    Component {
        prototype: "QQuickItem"
        name: "QtQuick.Controls/SplitView"
        exports: ["QtQuick.Controls/SplitView 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "__contents"
        Property { name: "orientation"; type: "int" }
        Property { name: "handleDelegate"; type: "QQmlComponent"; isPointer: true }
        Property { name: "resizing"; type: "bool" }
        Property { name: "__contents"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "__items"; type: "QQuickItem"; isList: true; isReadonly: true }
        Property { name: "__handles"; type: "QQuickItem"; isList: true; isReadonly: true }
    }
    Component {
        prototype: "QQuickItem"
        name: "QtQuick.Controls/StackView"
        exports: ["QtQuick.Controls/StackView 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "busy"; type: "bool"; isReadonly: true }
        Property { name: "delegate"; type: "StackViewDelegate_QMLTYPE_97"; isPointer: true }
        Property { name: "__currentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__depth"; type: "int" }
        Property { name: "__guard"; type: "bool" }
        Property { name: "invalidItemReplacement"; type: "QQmlComponent"; isPointer: true }
        Property { name: "initialItem"; type: "QVariant" }
        Property { name: "__currentTransition"; type: "QVariant" }
        Property { name: "depth"; type: "int"; isReadonly: true }
        Property { name: "currentItem"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Method {
            name: "push"
            type: "QVariant"
            Parameter { name: "item"; type: "QVariant" }
        }
        Method {
            name: "pop"
            type: "QVariant"
            Parameter { name: "item"; type: "QVariant" }
        }
        Method { name: "clear"; type: "QVariant" }
        Method {
            name: "find"
            type: "QVariant"
            Parameter { name: "func"; type: "QVariant" }
            Parameter { name: "onlySearchLoadedItems"; type: "QVariant" }
        }
        Method {
            name: "get"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
            Parameter { name: "dontLoad"; type: "QVariant" }
        }
        Method { name: "completeTransition"; type: "QVariant" }
        Method {
            name: "replace"
            type: "QVariant"
            Parameter { name: "item"; type: "QVariant" }
            Parameter { name: "properties"; type: "QVariant" }
            Parameter { name: "immediate"; type: "QVariant" }
        }
        Method {
            name: "__recursionGuard"
            type: "QVariant"
            Parameter { name: "use"; type: "QVariant" }
        }
        Method {
            name: "__loadElement"
            type: "QVariant"
            Parameter { name: "element"; type: "QVariant" }
        }
        Method {
            name: "__resolveComponent"
            type: "QVariant"
            Parameter { name: "unknownObjectType"; type: "QVariant" }
            Parameter { name: "element"; type: "QVariant" }
        }
        Method {
            name: "__cleanup"
            type: "QVariant"
            Parameter { name: "element"; type: "QVariant" }
        }
        Method {
            name: "__setStatus"
            type: "QVariant"
            Parameter { name: "item"; type: "QVariant" }
            Parameter { name: "status"; type: "QVariant" }
        }
        Method {
            name: "__performTransition"
            type: "QVariant"
            Parameter { name: "transition"; type: "QVariant" }
        }
        Method { name: "animationFinished"; type: "QVariant" }
    }
    Component {
        prototype: "QObject"
        name: "QtQuick.Controls/StackViewDelegate"
        exports: ["QtQuick.Controls/StackViewDelegate 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "pushTransition"; type: "QQmlComponent"; isPointer: true }
        Property { name: "popTransition"; type: "QQmlComponent"; isPointer: true }
        Property { name: "replaceTransition"; type: "QQmlComponent"; isPointer: true }
        Method {
            name: "getTransition"
            type: "QVariant"
            Parameter { name: "properties"; type: "QVariant" }
        }
        Method {
            name: "transitionFinished"
            type: "QVariant"
            Parameter { name: "properties"; type: "QVariant" }
        }
    }
    Component {
        prototype: "QQuickParallelAnimation"
        name: "QtQuick.Controls/StackViewTransition"
        exports: ["QtQuick.Controls/StackViewTransition 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "animations"
        Property { name: "name"; type: "string" }
        Property { name: "enterItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "exitItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "immediate"; type: "bool" }
    }
    Component {
        prototype: "QQuickItem"
        name: "QtQuick.Controls/StatusBar"
        exports: ["QtQuick.Controls/StatusBar 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "__content"
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isReadonly: true; isPointer: true }
        Property { name: "__content"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "contentItem"; type: "QQuickItem"; isReadonly: true; isPointer: true }
    }
    Component {
        prototype: "QQuickLoader"
        name: "QtQuick.Controls/Tab"
        exports: ["QtQuick.Controls/Tab 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "component"
        Property { name: "title"; type: "string" }
        Property { name: "__inserted"; type: "bool" }
        Property { name: "component"; type: "QQmlComponent"; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/TabView"
        exports: ["QtQuick.Controls/TabView 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "currentIndex"; type: "int" }
        Property { name: "count"; type: "int" }
        Property { name: "frameVisible"; type: "bool" }
        Property { name: "tabsVisible"; type: "bool" }
        Property { name: "tabPosition"; type: "int" }
        Property { name: "__tabs"; type: "QQmlListModel"; isPointer: true }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__styleItem"; type: "QVariant" }
        Property { name: "data"; type: "QObject"; isList: true; isReadonly: true }
        Method {
            name: "addTab"
            type: "QVariant"
            Parameter { name: "title"; type: "QVariant" }
            Parameter { name: "component"; type: "QVariant" }
        }
        Method {
            name: "insertTab"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
            Parameter { name: "title"; type: "QVariant" }
            Parameter { name: "component"; type: "QVariant" }
        }
        Method {
            name: "removeTab"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
        }
        Method {
            name: "moveTab"
            type: "QVariant"
            Parameter { name: "from"; type: "QVariant" }
            Parameter { name: "to"; type: "QVariant" }
        }
        Method {
            name: "getTab"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
        }
        Method { name: "__setOpacities"; type: "QVariant" }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/TableView"
        exports: ["QtQuick.Controls/TableView 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "__columns"
        Property { name: "frameVisible"; type: "bool" }
        Property { name: "highlightOnFocus"; type: "bool" }
        Property { name: "contentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__scroller"; type: "QQuickItem"; isPointer: true }
        Property { name: "__scrollBarTopMargin"; type: "int" }
        Property { name: "__viewTopMargin"; type: "int" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "Style_QMLTYPE_0"; isPointer: true }
        Property { name: "viewport"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Property { name: "flickableItem"; type: "QQuickFlickable"; isReadonly: true; isPointer: true }
        Property {
            name: "__horizontalScrollBar"
            type: "ScrollBar_QMLTYPE_51"
            isReadonly: true
            isPointer: true
        }
        Property {
            name: "__verticalScrollBar"
            type: "ScrollBar_QMLTYPE_51"
            isReadonly: true
            isPointer: true
        }
        Property { name: "alternatingRowColors"; type: "bool" }
        Property { name: "headerVisible"; type: "bool" }
        Property { name: "itemDelegate"; type: "QQmlComponent"; isPointer: true }
        Property { name: "rowDelegate"; type: "QQmlComponent"; isPointer: true }
        Property { name: "headerDelegate"; type: "QQmlComponent"; isPointer: true }
        Property { name: "sortIndicatorColumn"; type: "int" }
        Property { name: "sortIndicatorVisible"; type: "bool" }
        Property { name: "sortIndicatorOrder"; type: "int" }
        Property { name: "__activateItemOnSingleClick"; type: "bool" }
        Property { name: "model"; type: "QVariant" }
        Property { name: "backgroundVisible"; type: "bool" }
        Property { name: "__columns"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "contentHeader"; type: "QQmlComponent"; isPointer: true }
        Property { name: "contentFooter"; type: "QQmlComponent"; isPointer: true }
        Property { name: "rowCount"; type: "int"; isReadonly: true }
        Property { name: "columnCount"; type: "int"; isReadonly: true }
        Property { name: "section"; type: "QQuickViewSection"; isReadonly: true; isPointer: true }
        Property { name: "currentRow"; type: "int" }
        Property { name: "__currentRowItem"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Signal {
            name: "activated"
            Parameter { name: "row"; type: "int" }
        }
        Signal {
            name: "clicked"
            Parameter { name: "row"; type: "int" }
        }
        Signal {
            name: "doubleClicked"
            Parameter { name: "row"; type: "int" }
        }
        Method {
            name: "positionViewAtRow"
            type: "QVariant"
            Parameter { name: "row"; type: "QVariant" }
            Parameter { name: "mode"; type: "QVariant" }
        }
        Method {
            name: "rowAt"
            type: "QVariant"
            Parameter { name: "x"; type: "QVariant" }
            Parameter { name: "y"; type: "QVariant" }
        }
        Method {
            name: "addColumn"
            type: "QVariant"
            Parameter { name: "column"; type: "QVariant" }
        }
        Method {
            name: "insertColumn"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
            Parameter { name: "column"; type: "QVariant" }
        }
        Method {
            name: "removeColumn"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
        }
        Method {
            name: "moveColumn"
            type: "QVariant"
            Parameter { name: "from"; type: "QVariant" }
            Parameter { name: "to"; type: "QVariant" }
        }
        Method {
            name: "getColumn"
            type: "QVariant"
            Parameter { name: "index"; type: "QVariant" }
        }
        Method { name: "__decrementCurrentIndex"; type: "QVariant" }
        Method { name: "__incrementCurrentIndex"; type: "QVariant" }
    }
    Component {
        prototype: "QObject"
        name: "QtQuick.Controls/TableViewColumn"
        exports: ["QtQuick.Controls/TableViewColumn 1.0"]
        exportMetaObjectRevisions: [0]
        Property { name: "__view"; type: "QQuickItem"; isPointer: true }
        Property { name: "title"; type: "string" }
        Property { name: "role"; type: "string" }
        Property { name: "width"; type: "int" }
        Property { name: "visible"; type: "bool" }
        Property { name: "resizable"; type: "bool" }
        Property { name: "movable"; type: "bool" }
        Property { name: "elideMode"; type: "int" }
        Property { name: "horizontalAlignment"; type: "int" }
        Property { name: "delegate"; type: "QQmlComponent"; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/TextArea"
        exports: ["QtQuick.Controls/TextArea 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "frameVisible"; type: "bool" }
        Property { name: "highlightOnFocus"; type: "bool" }
        Property { name: "contentItem"; type: "QQuickItem"; isPointer: true }
        Property { name: "__scroller"; type: "QQuickItem"; isPointer: true }
        Property { name: "__scrollBarTopMargin"; type: "int" }
        Property { name: "__viewTopMargin"; type: "int" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "Style_QMLTYPE_0"; isPointer: true }
        Property { name: "viewport"; type: "QQuickItem"; isReadonly: true; isPointer: true }
        Property { name: "flickableItem"; type: "QQuickFlickable"; isReadonly: true; isPointer: true }
        Property {
            name: "__horizontalScrollBar"
            type: "ScrollBar_QMLTYPE_51"
            isReadonly: true
            isPointer: true
        }
        Property {
            name: "__verticalScrollBar"
            type: "ScrollBar_QMLTYPE_51"
            isReadonly: true
            isPointer: true
        }
        Property { name: "tabChangesFocus"; type: "bool" }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "baseUrl"; type: "QUrl" }
        Property { name: "canPaste"; type: "bool"; isReadonly: true }
        Property { name: "canRedo"; type: "bool"; isReadonly: true }
        Property { name: "canUndo"; type: "bool"; isReadonly: true }
        Property { name: "textColor"; type: "QColor" }
        Property { name: "cursorPosition"; type: "int" }
        Property { name: "font"; type: "QFont" }
        Property { name: "horizontalAlignment"; type: "int" }
        Property { name: "effectiveHorizontalAlignment"; type: "int"; isReadonly: true }
        Property { name: "verticalAlignment"; type: "int" }
        Property { name: "inputMethodHints"; type: "int" }
        Property { name: "length"; type: "int"; isReadonly: true }
        Property { name: "lineCount"; type: "int"; isReadonly: true }
        Property { name: "readOnly"; type: "bool" }
        Property { name: "selectedText"; type: "string"; isReadonly: true }
        Property { name: "selectionEnd"; type: "int"; isReadonly: true }
        Property { name: "selectionStart"; type: "int"; isReadonly: true }
        Property { name: "text"; type: "string" }
        Property { name: "textFormat"; type: "int" }
        Property { name: "wrapMode"; type: "int" }
        Property { name: "selectByMouse"; type: "bool" }
        Property { name: "selectByKeyboard"; type: "bool" }
        Property { name: "hoveredLink"; type: "string"; isReadonly: true }
        Property { name: "backgroundVisible"; type: "bool" }
        Property { name: "data"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "textMargin"; type: "double" }
        Property { name: "textDocument"; type: "QQuickTextDocument"; isReadonly: true; isPointer: true }
        Signal {
            name: "linkActivated"
            Parameter { name: "link"; type: "string" }
        }
        Signal {
            name: "linkHovered"
            Parameter { name: "link"; type: "string" }
        }
        Method {
            name: "append"
            type: "QVariant"
            Parameter { name: "string"; type: "QVariant" }
        }
        Method { name: "copy"; type: "QVariant" }
        Method { name: "cut"; type: "QVariant" }
        Method { name: "deselect"; type: "QVariant" }
        Method {
            name: "getFormattedText"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method {
            name: "getText"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method {
            name: "insert"
            type: "QVariant"
            Parameter { name: "position"; type: "QVariant" }
            Parameter { name: "text"; type: "QVariant" }
        }
        Method {
            name: "isRightToLeft"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method {
            name: "moveCursorSelection"
            type: "QVariant"
            Parameter { name: "position"; type: "QVariant" }
            Parameter { name: "mode"; type: "QVariant" }
        }
        Method { name: "paste"; type: "QVariant" }
        Method {
            name: "positionAt"
            type: "QVariant"
            Parameter { name: "x"; type: "QVariant" }
            Parameter { name: "y"; type: "QVariant" }
        }
        Method {
            name: "positionToRectangle"
            type: "QVariant"
            Parameter { name: "position"; type: "QVariant" }
        }
        Method { name: "redo"; type: "QVariant" }
        Method {
            name: "remove"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method {
            name: "select"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method { name: "selectAll"; type: "QVariant" }
        Method { name: "selectWord"; type: "QVariant" }
        Method { name: "undo"; type: "QVariant" }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/TextField"
        exports: ["QtQuick.Controls/TextField 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
        Property { name: "acceptableInput"; type: "bool"; isReadonly: true }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "canPaste"; type: "bool"; isReadonly: true }
        Property { name: "canRedo"; type: "bool"; isReadonly: true }
        Property { name: "canUndo"; type: "bool"; isReadonly: true }
        Property { name: "textColor"; type: "QColor" }
        Property { name: "cursorPosition"; type: "int" }
        Property { name: "displayText"; type: "string"; isReadonly: true }
        Property { name: "echoMode"; type: "int" }
        Property { name: "font"; type: "QFont" }
        Property { name: "horizontalAlignment"; type: "int" }
        Property { name: "effectiveHorizontalAlignment"; type: "int"; isReadonly: true }
        Property { name: "verticalAlignment"; type: "int" }
        Property { name: "inputMask"; type: "string" }
        Property { name: "inputMethodHints"; type: "int" }
        Property { name: "length"; type: "int"; isReadonly: true }
        Property { name: "maximumLength"; type: "int" }
        Property { name: "placeholderText"; type: "string" }
        Property { name: "readOnly"; type: "bool" }
        Property { name: "selectedText"; type: "string"; isReadonly: true }
        Property { name: "selectionEnd"; type: "int"; isReadonly: true }
        Property { name: "selectionStart"; type: "int"; isReadonly: true }
        Property { name: "text"; type: "string" }
        Property { name: "validator"; type: "QValidator"; isPointer: true }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Property { name: "__contentHeight"; type: "double"; isReadonly: true }
        Property { name: "__contentWidth"; type: "double"; isReadonly: true }
        Signal { name: "accepted" }
        Method { name: "copy"; type: "QVariant" }
        Method { name: "cut"; type: "QVariant" }
        Method { name: "deselect"; type: "QVariant" }
        Method {
            name: "getText"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method {
            name: "insert"
            type: "QVariant"
            Parameter { name: "position"; type: "QVariant" }
            Parameter { name: "text"; type: "QVariant" }
        }
        Method {
            name: "isRightToLeft"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method { name: "paste"; type: "QVariant" }
        Method { name: "redo"; type: "QVariant" }
        Method {
            name: "select"
            type: "QVariant"
            Parameter { name: "start"; type: "QVariant" }
            Parameter { name: "end"; type: "QVariant" }
        }
        Method { name: "selectAll"; type: "QVariant" }
        Method { name: "selectWord"; type: "QVariant" }
        Method { name: "undo"; type: "QVariant" }
    }
    Component {
        prototype: "QQuickItem"
        name: "QtQuick.Controls/ToolBar"
        exports: ["QtQuick.Controls/ToolBar 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "__content"
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isReadonly: true; isPointer: true }
        Property { name: "__content"; type: "QObject"; isList: true; isReadonly: true }
        Property { name: "contentItem"; type: "QQuickItem"; isReadonly: true; isPointer: true }
    }
    Component {
        prototype: "QQuickFocusScope"
        name: "QtQuick.Controls/ToolButton"
        exports: ["QtQuick.Controls/ToolButton 1.0"]
        exportMetaObjectRevisions: [0]
        defaultProperty: "data"
        Property { name: "checkable"; type: "bool" }
        Property { name: "checked"; type: "bool" }
        Property { name: "exclusiveGroup"; type: "QQuickExclusiveGroup"; isPointer: true }
        Property { name: "action"; type: "QQuickAction"; isPointer: true }
        Property { name: "activeFocusOnPress"; type: "bool" }
        Property { name: "text"; type: "string" }
        Property { name: "tooltip"; type: "string" }
        Property { name: "iconSource"; type: "QUrl" }
        Property { name: "iconName"; type: "string" }
        Property { name: "__textColor"; type: "QColor" }
        Property { name: "__position"; type: "string" }
        Property { name: "__iconOverriden"; type: "bool"; isReadonly: true }
        Property { name: "__action"; type: "QQuickAction"; isPointer: true }
        Property { name: "__iconAction"; type: "QQuickAction"; isReadonly: true; isPointer: true }
        Property { name: "__effectivePressed"; type: "bool" }
        Property { name: "__behavior"; type: "QVariant" }
        Property { name: "pressed"; type: "bool"; isReadonly: true }
        Property { name: "hovered"; type: "bool"; isReadonly: true }
        Signal { name: "clicked" }
        Method { name: "accessiblePressAction"; type: "QVariant" }
        Property { name: "style"; type: "QQmlComponent"; isPointer: true }
        Property { name: "__style"; type: "QObject"; isPointer: true }
        Property { name: "__panel"; type: "QQuickItem"; isPointer: true }
        Property { name: "styleHints"; type: "QVariant" }
        Property { name: "__styleData"; type: "QObject"; isPointer: true }
    }
}
