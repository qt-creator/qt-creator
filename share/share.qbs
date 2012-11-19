import qbs.base 1.0

Product {
    type: ["installed_content"]
    name: "SharedContent"

    Group {
        condition: qbs.targetOS == "macx"
        qbs.installDir: "share/qtcreator/scripts"
        fileTags: ["install"]
        files: "qtcreator/scripts/openTerminal.command"
    }

    Group {
        qbs.installDir: "share/qtcreator/designer"
        fileTags: ["install"]
        prefix: "qtcreator/designer/"
        files: [
            "templates.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/designer/templates"
        fileTags: ["install"]
        prefix: "qtcreator/designer/templates/"
        files: [
            "Dialog_with_Buttons_Bottom.ui",
            "Dialog_with_Buttons_Right.ui",
            "Dialog_without_Buttons.ui",
            "Main_Window.ui",
            "Widget.ui",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/dumper"
        fileTags: ["install"]
        prefix: "qtcreator/dumper/"
        files: [
            "LGPL_EXCEPTION.TXT",
            "LICENSE.LGPL",
            "bridge.py",
            "dumper.cpp",
            "dumper.h",
            "dumper.pro",
            "dumper.py",
            "dumper_p.h",
            "pdumper.py",
            "qttypes.py",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/dumper/test"
        fileTags: ["install"]
        prefix: "qtcreator/dumper/test/"
        files: [
            "dumpertest.pro",
            "main.cpp",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/generic-highlighter"
        fileTags: ["install"]
        prefix: "qtcreator/generic-highlighter/"
        files: [
            "alert.xml",
            "autoconf.xml",
            "bash.xml",
            "cmake.xml",
            "css.xml",
            "doxygen.xml",
            "dtd.xml",
            "html.xml",
            "ini.xml",
            "java.xml",
            "javadoc.xml",
            "perl.xml",
            "ruby.xml",
            "valgrind-suppression.xml",
            "xml.xml",
            "yacc.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/glsl"
        fileTags: ["install"]
        prefix: "qtcreator/glsl/"
        files: [
            "glsl_120.frag",
            "glsl_120.vert",
            "glsl_120_common.glsl",
            "glsl_es_100.frag",
            "glsl_es_100.vert",
            "glsl_es_100_common.glsl",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml-type-descriptions"
        fileTags: ["install"]
        prefix: "qtcreator/qml-type-descriptions/"
        files: [
            "builtins.qmltypes",
            "qmlproject.qmltypes",
            "qmlruntime.qmltypes",
            "qt-labs-folderlistmodel.qmltypes",
            "qt-labs-gestures.qmltypes",
            "qt-labs-particles.qmltypes",
            "qtmobility-connectivity.qmltypes",
            "qtmobility-contacts.qmltypes",
            "qtmobility-feedback.qmltypes",
            "qtmobility-gallery.qmltypes",
            "qtmobility-location.qmltypes",
            "qtmobility-messaging.qmltypes",
            "qtmobility-organizer.qmltypes",
            "qtmobility-publishsubscribe.qmltypes",
            "qtmobility-sensors.qmltypes",
            "qtmobility-serviceframework.qmltypes",
            "qtmobility-systeminfo.qmltypes",
            "qtmultimediakit.qmltypes",
            "qtwebkit.qmltypes",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmldump"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmldump/"
        files: [
            "LGPL_EXCEPTION.TXT",
            "LICENSE.LGPL",
            "main.cpp",
            "qmldump.pro",
            "qmlstreamwriter.cpp",
            "qmlstreamwriter.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmljsdebugger/"
        files: [
            "jsdebuggeragent.cpp",
            "qdeclarativeinspectorservice.cpp",
            "qdeclarativeviewinspector.cpp",
            "qdeclarativeviewinspector_p.h",
            "qmljsdebugger-lib.pri",
            "qmljsdebugger-src.pri",
            "qmljsdebugger.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/editor"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmljsdebugger/editor/"
        files: [
            "abstractliveedittool.cpp",
            "abstractliveedittool.h",
            "boundingrecthighlighter.cpp",
            "boundingrecthighlighter.h",
            "colorpickertool.cpp",
            "colorpickertool.h",
            "livelayeritem.cpp",
            "livelayeritem.h",
            "liverubberbandselectionmanipulator.cpp",
            "liverubberbandselectionmanipulator.h",
            "liveselectionindicator.cpp",
            "liveselectionindicator.h",
            "liveselectionrectangle.cpp",
            "liveselectionrectangle.h",
            "liveselectiontool.cpp",
            "liveselectiontool.h",
            "livesingleselectionmanipulator.cpp",
            "livesingleselectionmanipulator.h",
            "subcomponentmasklayeritem.cpp",
            "subcomponentmasklayeritem.h",
            "zoomtool.cpp",
            "zoomtool.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/include"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmljsdebugger/include/"
        files: [
            "jsdebuggeragent.h",
            "qdeclarativeinspectorservice.h",
            "qdeclarativeviewinspector.h",
            "qdeclarativeviewobserver.h",
            "qmlinspectorconstants.h",
            "qmljsdebugger_global.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/include/qt_private"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmljsdebugger/include/qt_private/"
        files: [
            "qdeclarativedebughelper_p.h",
            "qdeclarativedebugservice_p.h",
            "qdeclarativestate_p.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/protocol"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmljsdebugger/protocol/"
        files: [
            "inspectorprotocol.h",
            "protocol.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlobserver/"
        files: [
            "LGPL_EXCEPTION.TXT",
            "LICENSE.LGPL",
            "deviceorientation.cpp",
            "deviceorientation.h",
            "deviceorientation_harmattan.cpp",
            "deviceorientation_maemo5.cpp",
            "deviceorientation_symbian.cpp",
            "loggerwidget.cpp",
            "loggerwidget.h",
            "main.cpp",
            "proxysettings.cpp",
            "proxysettings.h",
            "proxysettings.ui",
            "proxysettings_maemo5.ui",
            "qdeclarativetester.cpp",
            "qdeclarativetester.h",
            "qml.icns",
            "qml.pri",
            "qmlobserver.pro",
            "qmlruntime.cpp",
            "qmlruntime.h",
            "recopts.ui",
            "recopts_maemo5.ui",
            "texteditautoresizer_maemo5.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver/browser"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlobserver/browser/"
        files: [
            "Browser.qml",
            "browser.qrc",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver/browser/images"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlobserver/browser/images/"
        files: [
            "folder.png",
            "titlebar.png",
            "titlebar.sci",
            "up.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver/startup"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlobserver/startup/"
        files: [
            "Logo.qml",
            "qt-back.png",
            "qt-blue.jpg",
            "qt-front.png",
            "qt-sketch.jpg",
            "qt-text.png",
            "quick-blur.png",
            "quick-regular.png",
            "shadow.png",
            "startup.qml",
            "startup.qrc",
            "white-star.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/"
        files: [
            "qmlpuppet.pro",
            "qmlpuppet.qrc",
            "qmlpuppet_utilities.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/commands"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/commands/"
        files: [
            "changeauxiliarycommand.cpp",
            "changeauxiliarycommand.h",
            "changebindingscommand.cpp",
            "changebindingscommand.h",
            "changefileurlcommand.cpp",
            "changefileurlcommand.h",
            "changeidscommand.cpp",
            "changeidscommand.h",
            "changenodesourcecommand.cpp",
            "changenodesourcecommand.h",
            "changestatecommand.cpp",
            "changestatecommand.h",
            "changevaluescommand.cpp",
            "changevaluescommand.h",
            "childrenchangedcommand.cpp",
            "childrenchangedcommand.h",
            "clearscenecommand.cpp",
            "clearscenecommand.h",
            "commands.pri",
            "completecomponentcommand.cpp",
            "completecomponentcommand.h",
            "componentcompletedcommand.cpp",
            "componentcompletedcommand.h",
            "createinstancescommand.cpp",
            "createinstancescommand.h",
            "createscenecommand.cpp",
            "createscenecommand.h",
            "informationchangedcommand.cpp",
            "informationchangedcommand.h",
            "pixmapchangedcommand.cpp",
            "pixmapchangedcommand.h",
            "removeinstancescommand.cpp",
            "removeinstancescommand.h",
            "removepropertiescommand.cpp",
            "removepropertiescommand.h",
            "reparentinstancescommand.cpp",
            "reparentinstancescommand.h",
            "statepreviewimagechangedcommand.cpp",
            "statepreviewimagechangedcommand.h",
            "synchronizecommand.cpp",
            "synchronizecommand.h",
            "tokencommand.cpp",
            "tokencommand.h",
            "valueschangedcommand.cpp",
            "valueschangedcommand.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/container"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/container/"
        files: [
            "addimportcontainer.cpp",
            "addimportcontainer.h",
            "container.pri",
            "idcontainer.cpp",
            "idcontainer.h",
            "imagecontainer.cpp",
            "imagecontainer.h",
            "informationcontainer.cpp",
            "informationcontainer.h",
            "instancecontainer.cpp",
            "instancecontainer.h",
            "propertyabstractcontainer.cpp",
            "propertyabstractcontainer.h",
            "propertybindingcontainer.cpp",
            "propertybindingcontainer.h",
            "propertyvaluecontainer.cpp",
            "propertyvaluecontainer.h",
            "reparentcontainer.cpp",
            "reparentcontainer.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/html"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/html/"
        files: [
            "welcome.html",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/images"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/images/"
        files: [
            "template_image.png",
            "webkit.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/instances"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/instances/"
        files: [
            "anchorchangesnodeinstance.cpp",
            "anchorchangesnodeinstance.h",
            "behaviornodeinstance.cpp",
            "behaviornodeinstance.h",
            "childrenchangeeventfilter.cpp",
            "childrenchangeeventfilter.h",
            "componentnodeinstance.cpp",
            "componentnodeinstance.h",
            "dummycontextobject.cpp",
            "dummycontextobject.h",
            "dummynodeinstance.cpp",
            "dummynodeinstance.h",
            "instances.pri",
            "nodeinstanceclientproxy.cpp",
            "nodeinstanceclientproxy.h",
            "nodeinstancemetaobject.cpp",
            "nodeinstancemetaobject.h",
            "nodeinstanceserver.cpp",
            "nodeinstanceserver.h",
            "nodeinstancesignalspy.cpp",
            "nodeinstancesignalspy.h",
            "objectnodeinstance.cpp",
            "objectnodeinstance.h",
            "qmlpropertychangesnodeinstance.cpp",
            "qmlpropertychangesnodeinstance.h",
            "qmlstatenodeinstance.cpp",
            "qmlstatenodeinstance.h",
            "qmltransitionnodeinstance.cpp",
            "qmltransitionnodeinstance.h",
            "servernodeinstance.cpp",
            "servernodeinstance.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/interfaces"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/interfaces/"
        files: [
            "commondefines.h",
            "interfaces.pri",
            "nodeinstanceclientinterface.h",
            "nodeinstanceserverinterface.cpp",
            "nodeinstanceserverinterface.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qml2puppet"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/qml2puppet/"
        files: [
            "main.cpp",
            "qml2puppet.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qml2puppet/instances"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/qml2puppet/instances/"
        files: [
            "instances.pri",
            "qt5informationnodeinstanceserver.cpp",
            "qt5informationnodeinstanceserver.h",
            "qt5nodeinstanceclientproxy.cpp",
            "qt5nodeinstanceclientproxy.h",
            "qt5nodeinstanceserver.cpp",
            "qt5nodeinstanceserver.h",
            "qt5previewnodeinstanceserver.cpp",
            "qt5previewnodeinstanceserver.h",
            "qt5rendernodeinstanceserver.cpp",
            "qt5rendernodeinstanceserver.h",
            "sgitemnodeinstance.cpp",
            "sgitemnodeinstance.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qmlpuppet"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/qmlpuppet/"
        files: [
            "main.cpp",
            "qmlpuppet.pri",
            "qmlpuppet.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qmlpuppet/instances"
        fileTags: ["install"]
        prefix: "qtcreator/qml/qmlpuppet/qmlpuppet/instances/"
        files: [
            "graphicsobjectnodeinstance.cpp",
            "graphicsobjectnodeinstance.h",
            "instances.pri",
            "positionernodeinstance.cpp",
            "positionernodeinstance.h",
            "qmlgraphicsitemnodeinstance.cpp",
            "qmlgraphicsitemnodeinstance.h",
            "qt4informationnodeinstanceserver.cpp",
            "qt4informationnodeinstanceserver.h",
            "qt4nodeinstanceclientproxy.cpp",
            "qt4nodeinstanceclientproxy.h",
            "qt4nodeinstanceserver.cpp",
            "qt4nodeinstanceserver.h",
            "qt4previewnodeinstanceserver.cpp",
            "qt4previewnodeinstanceserver.h",
            "qt4rendernodeinstanceserver.cpp",
            "qt4rendernodeinstanceserver.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmldesigner/propertyeditor/Qt"
        fileTags: ["install"]
        prefix: "qtcreator/qmldesigner/propertyeditor/Qt/"
        files: [
            "AlignmentHorizontalButtons.qml",
            "AlignmentVerticalButtons.qml",
            "AnchorBox.qml",
            "AnchorButtons.qml",
            "BorderImageSpecifics.qml",
            "CheckBox.qml",
            "ColorGroupBox.qml",
            "ColorLabel.qml",
            "ColorScheme.qml",
            "ColorTypeButtons.qml",
            "ComboBox.qml",
            "DoubleSpinBox.qml",
            "DoubleSpinBoxAlternate.qml",
            "ExpressionEditor.qml",
            "Extended.qml",
            "ExtendedFunctionButton.qml",
            "ExtendedPane.qml",
            "ExtendedSwitches.qml",
            "FlagedButton.qml",
            "FlickableGroupBox.qml",
            "FlickableSpecifics.qml",
            "FlipableSpecifics.qml",
            "FlowSpecifics.qml",
            "FontComboBox.qml",
            "FontGroupBox.qml",
            "FontStyleButtons.qml",
            "Geometry.qml",
            "GridSpecifics.qml",
            "GridViewSpecifics.qml",
            "GroupBox.qml",
            "GroupBoxOption.qml",
            "HorizontalLayout.qml",
            "HorizontalWhiteLine.qml",
            "ImageSpecifics.qml",
            "IntEditor.qml",
            "ItemPane.qml",
            "Label.qml",
            "Layout.qml",
            "LayoutPane.qml",
            "LineEdit.qml",
            "ListViewSpecifics.qml",
            "Modifiers.qml",
            "MouseAreaSpecifics.qml",
            "PathViewSpecifics.qml",
            "PlaceHolder.qml",
            "PropertyFrame.qml",
            "RectangleColorGroupBox.qml",
            "RectangleSpecifics.qml",
            "RowSpecifics.qml",
            "ScrollArea.qml",
            "SliderWidget.qml",
            "SpinBox.qml",
            "StandardTextColorGroupBox.qml",
            "StandardTextGroupBox.qml",
            "Switches.qml",
            "TextEditSpecifics.qml",
            "TextEditor.qml",
            "TextInputGroupBox.qml",
            "TextInputSpecifics.qml",
            "TextSpecifics.qml",
            "Transformation.qml",
            "Type.qml",
            "VerticalLayout.qml",
            "Visibility.qml",
            "anchorbottom.css",
            "anchorbox.css",
            "anchorfill.css",
            "anchorhorizontal.css",
            "anchorleft.css",
            "anchorright.css",
            "anchorspacer.css",
            "anchortop.css",
            "anchorvertical.css",
            "applybutton.css",
            "aspectlock.css",
            "cancelbutton.css",
            "checkbox_tr.css",
            "emptyPane.qml",
            "layoutWidget.css",
            "propertyEditor.css",
            "specialCheckBox.css",
            "styledbuttonleft.css",
            "styledbuttonmiddle.css",
            "styledbuttonright.css",
            "switch.css",
            "typeLabel.css",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmldesigner/propertyeditor/Qt/images"
        fileTags: ["install"]
        prefix: "qtcreator/qmldesigner/propertyeditor/Qt/images/"
        files: [
            "alignmentbottom-h-icon.png",
            "alignmentbottom-icon.png",
            "alignmentcenterh-h-icon.png",
            "alignmentcenterh-icon.png",
            "alignmentleft-h-icon.png",
            "alignmentleft-icon.png",
            "alignmentmiddle-h-icon.png",
            "alignmentmiddle-icon.png",
            "alignmentright-h-icon.png",
            "alignmentright-icon.png",
            "alignmenttop-h-icon.png",
            "alignmenttop-icon.png",
            "apply.png",
            "behaivour.png",
            "blended-image-icon.png",
            "bold-h-icon.png",
            "bold-icon.png",
            "button.png",
            "cancel.png",
            "default-icon.png",
            "downArrow.png",
            "expression.png",
            "extended.png",
            "grid-icon.png",
            "icon_color_gradient.png",
            "icon_color_none.png",
            "icon_color_solid.png",
            "image-icon.png",
            "italic-h-icon.png",
            "italic-icon.png",
            "item-icon.png",
            "layout.png",
            "leftArrow.png",
            "list-icon.png",
            "mouse-area-icon.png",
            "placeholder.png",
            "rect-icon.png",
            "reset-button.png",
            "rightArrow.png",
            "standard.png",
            "strikeout-h-icon.png",
            "strikeout-icon.png",
            "submenu.png",
            "text-edit-icon.png",
            "text-icon.png",
            "underline-h-icon.png",
            "underline-icon.png",
            "upArrow.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmldesigner/propertyeditor/QtWebKit"
        fileTags: ["install"]
        prefix: "qtcreator/qmldesigner/propertyeditor/QtWebKit/"
        files: [
            "WebViewSpecifics.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmlicons/Qt/16x16"
        fileTags: ["install"]
        prefix: "qtcreator/qmlicons/Qt/16x16/"
        files: [
            "BorderImage.png",
            "BusyIndicator.png",
            "Button.png",
            "ButtonColumn.png",
            "ButtonRow.png",
            "CheckBox.png",
            "ChoiceList.png",
            "ColorAnimation.png",
            "Component.png",
            "CountBubble.png",
            "DatePickerDialog.png",
            "Flickable.png",
            "Flipable.png",
            "FocusScope.png",
            "GridView.png",
            "Image.png",
            "InfoBanner.png",
            "Item.png",
            "ListButton.png",
            "ListDelegate.png",
            "ListView.png",
            "MoreIndicator.png",
            "MouseArea.png",
            "PageIndicator.png",
            "ParallelAnimation.png",
            "PathView.png",
            "PauseAnimation.png",
            "ProgressBar.png",
            "PropertyChanges.png",
            "RadioButton.png",
            "RatingIndicator.png",
            "Rectangle.png",
            "SequentialAnimation.png",
            "Slider.png",
            "State.png",
            "Switch.png",
            "TabBar.png",
            "TabButton.png",
            "Text.png",
            "TextArea.png",
            "TextEdit.png",
            "TextField.png",
            "TextInput.png",
            "TimePickerDialog.png",
            "ToolBar.png",
            "Transition.png",
            "Tumbler.png",
            "TumblerButton.png",
            "TumblerColumn.png",
            "TumblerDialog.png",
            "Window.png",
            "item-icon16.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmlicons/QtWebkit/16x16"
        fileTags: ["install"]
        prefix: "qtcreator/qmlicons/QtWebkit/16x16/"
        files: [
            "WebView.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/schemes"
        fileTags: ["install"]
        prefix: "qtcreator/schemes/"
        files: [
            "MS_Visual_C++.kms",
            "Xcode.kms",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/snippets"
        fileTags: ["install"]
        prefix: "qtcreator/snippets/"
        files: [
            "cpp.xml",
            "qml.xml",
            "text.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/styles"
        fileTags: ["install"]
        prefix: "qtcreator/styles/"
        files: [
            "darkvim.xml",
            "default.xml",
            "grayscale.xml",
            "inkpot.xml",
            "intellij.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app"
        fileTags: ["install"]
        prefix: "qtcreator/templates/html5app/"
        files: [
            "app.pro",
            "main.cpp",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app/html"
        fileTags: ["install"]
        prefix: "qtcreator/templates/html5app/html/"
        files: [
            "index.html",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app/html5applicationviewer"
        fileTags: ["install"]
        prefix: "qtcreator/templates/html5app/html5applicationviewer/"
        files: [
            "html5applicationviewer.cpp",
            "html5applicationviewer.h",
            "html5applicationviewer.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app/html5applicationviewer/touchnavigation"
        fileTags: ["install"]
        prefix: "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/"
        files: [
            "navigationcontroller.cpp",
            "navigationcontroller.h",
            "touchnavigation.pri",
            "webnavigation.cpp",
            "webnavigation.h",
            "webtouchevent.cpp",
            "webtouchevent.h",
            "webtouchnavigation.cpp",
            "webtouchnavigation.h",
            "webtouchphysics.cpp",
            "webtouchphysics.h",
            "webtouchphysicsinterface.cpp",
            "webtouchphysicsinterface.h",
            "webtouchscroller.cpp",
            "webtouchscroller.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/mobileapp"
        fileTags: ["install"]
        prefix: "qtcreator/templates/mobileapp/"
        files: [
            "app.pro",
            "main.cpp",
            "mainwindow.cpp",
            "mainwindow.h",
            "mainwindow.ui",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qt4project"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qt4project/"
        files: [
            "main.cpp",
            "mywidget.cpp",
            "mywidget.h",
            "mywidget_form.cpp",
            "mywidget_form.h",
            "widget.ui",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qt4project/customwidgetwizard"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qt4project/customwidgetwizard/"
        files: [
            "tpl_collection.cpp",
            "tpl_collection.h",
            "tpl_plugin.pro",
            "tpl_resources.qrc",
            "tpl_single.cpp",
            "tpl_single.h",
            "tpl_widget.cpp",
            "tpl_widget.h",
            "tpl_widget_include.pri",
            "tpl_widget_lib.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquick2app"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qtquick2app/"
        files: [
            "app.pro",
            "main.cpp",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquick2app/qml/app/qtquick20"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qtquick2app/qml/app/qtquick20/"
        files: [
            "main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquick2app/qtquick2applicationviewer"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qtquick2app/qtquick2applicationviewer/"
        files: [
            "qtquick2applicationviewer.cpp",
            "qtquick2applicationviewer.h",
            "qtquick2applicationviewer.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qtquickapp/"
        files: [
            "app.pro",
            "main.cpp",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp/qml/app/meego10"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qtquickapp/qml/app/meego10/"
        files: [
            "MainPage.qml",
            "main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp/qml/app/qtquick10"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qtquickapp/qml/app/qtquick10/"
        files: [
            "main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp/qmlapplicationviewer"
        fileTags: ["install"]
        prefix: "qtcreator/templates/qtquickapp/qmlapplicationviewer/"
        files: [
            "qmlapplicationviewer.cpp",
            "qmlapplicationviewer.h",
            "qmlapplicationviewer.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/shared"
        fileTags: ["install"]
        prefix: "qtcreator/templates/shared/"
        files: [
            "app.desktop",
            "deployment.pri",
            "icon64.png",
            "icon80.png",
            "manifest.aegis",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/"
        files: [
            "README.txt",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-bardescriptor"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-bardescriptor/"
        files: [
            "bar-descriptor.xml",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-guiapp"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-guiapp/"
        files: [
            "bar-descriptor.xml",
            "icon.png",
            "main.cpp",
            "mainwidget.cpp",
            "mainwidget.h",
            "mainwidget.ui",
            "project.pro",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-qt5-bardescriptor"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-qt5-bardescriptor/"
        files: [
            "bar-descriptor.xml",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-qt5-guiapp"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-qt5-guiapp/"
        files: [
            "bar-descriptor.xml",
            "icon.png",
            "main.cpp",
            "mainwidget.cpp",
            "mainwidget.h",
            "mainwidget.ui",
            "project.pro",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-qt5-quick2app"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-qt5-quick2app/"
        files: [
            "bar-descriptor.xml",
            "icon.png",
            "main.cpp",
            "project.pro",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-qt5-quick2app/qml"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-qt5-quick2app/qml/"
        files: [
            "main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-quickapp"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-quickapp/"
        files: [
            "bar-descriptor.xml",
            "icon.png",
            "main.cpp",
            "project.pro",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/bb-quickapp/qml"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/bb-quickapp/qml/"
        files: [
            "main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/helloworld"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/helloworld/"
        files: [
            "console.png",
            "main.cpp",
            "project.pro",
            "wizard_sample.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/listmodel"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/listmodel/"
        files: [
            "listmodel.cpp",
            "listmodel.h",
            "wizard_sample.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincapp"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/plaincapp/"
        files: [
            "console.png",
            "main.c",
            "project.pro",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincapp-cmake"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/plaincapp-cmake/"
        files: [
            "CMakeLists.txt",
            "console.png",
            "main.c",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincppapp"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/plaincppapp/"
        files: [
            "console.png",
            "main.cpp",
            "project.pro",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincppapp-cmake"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/plaincppapp-cmake/"
        files: [
            "CMakeLists.txt",
            "console.png",
            "main.cpp",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/qtquick1-extension"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/qtquick1-extension/"
        files: [
            "lib.png",
            "object.cpp",
            "object.h",
            "plugin.cpp",
            "plugin.h",
            "project.pro",
            "qmldir",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/qtquick2-extension"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/qtquick2-extension/"
        files: [
            "lib.png",
            "object.cpp",
            "object.h",
            "plugin.cpp",
            "plugin.h",
            "project.pro",
            "qmldir",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/qtcreatorplugin"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/qtcreatorplugin/"
        files: [
            "MyPlugin.pluginspec.in",
            "myplugin.cpp",
            "myplugin.h",
            "myplugin.pro",
            "myplugin_global.h",
            "mypluginconstants.h",
            "qtcreator_logo_24.png",
            "wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/scriptgeneratedproject"
        fileTags: ["install"]
        prefix: "qtcreator/templates/wizards/scriptgeneratedproject/"
        files: [
            "generate.pl",
            "wizard_sample.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/"
        files: [
            "develop.qml",
            "examples.qml",
            "examples_fallback.xml",
            "gettingstarted.qml",
            "images_areaofinterest.xml",
            "qtcreator_tutorials.xml",
            "tutorials.qml",
            "welcomescreen.qml",
            "welcomescreen.qmlproject",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/dummydata"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/dummydata/"
        files: [
            "examplesModel.qml",
            "pagesModel.qml",
            "projectList.qml",
            "sessionList.qml",
            "tutorialsModel.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/widgets/"
        files: [
            "CustomColors.qml",
            "CustomFonts.qml",
            "CustomTab.qml",
            "CustomizedGridView.qml",
            "Delegate.qml",
            "GettingStartedItem.qml",
            "IconAndLink.qml",
            "LinkedText.qml",
            "LinksBar.qml",
            "Logo.qml",
            "PageCaption.qml",
            "PageLoader.qml",
            "ProjectItem.qml",
            "RecentProjects.qml",
            "SearchBar.qml",
            "SessionItem.qml",
            "Sessions.qml",
            "ToolTip.qml",
            "qmldir",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/dummydata"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/widgets/dummydata/"
        files: [
            "examplesModel.qml",
            "mockupTags.qml",
            "pagesModel.qml",
            "tabsModel.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/dummydata/context"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/widgets/dummydata/context/"
        files: [
            "ExampleDelegate.qml",
            "ExampleGridView.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/images"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/widgets/images/"
        files: [
            "arrowBig.png",
            "arrow_down.png",
            "arrow_up.png",
            "bullet.png",
            "dropshadow.png",
            "gettingStarted01.png",
            "gettingStarted02.png",
            "gettingStarted03.png",
            "gettingStarted04.png",
            "info.png",
            "more.png",
            "qtcreator.png",
            "tab.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/images/icons"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/widgets/images/icons/"
        files: [
            "adressbook.png",
            "buildrun.png",
            "clone.png",
            "communityIcon.png",
            "components.png",
            "createIcon.png",
            "ddays09.png",
            "ddays10.png",
            "ddays11.png",
            "delete.png",
            "developing_with_qt_creator.png",
            "feedbackIcon.png",
            "ico_community.png",
            "labsIcon.png",
            "openIcon.png",
            "qt_quick_1.png",
            "qt_quick_2.png",
            "qt_quick_3.png",
            "qt_sdk.png",
            "qtquick.png",
            "qwidget.png",
            "rename.png",
            "userguideIcon.png",
            "videoIcon.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/images/mockup"
        fileTags: ["install"]
        prefix: "qtcreator/welcomescreen/widgets/images/mockup/"
        files: [
            "designer-examples.png",
            "desktop-examples.png",
            "draganddrop-examples.png",
            "itemview-examples.png",
            "layout-examples.png",
            "mainwindow-examples.png",
            "network-examples.png",
            "opengl-examples.png",
            "penguin.png",
            "qtscript-examples.png",
            "thread-examples.png",
        ]
    }
}

