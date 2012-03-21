import qbs.base 1.0

Product {
    type: ["installed_content"]
    name: "SharedContent"

    Group {
        qbs.installDir: "share/qtcreator/designer"
        fileTags: ["install"]
        files: [
            "qtcreator/designer/templates.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/designer/templates"
        fileTags: ["install"]
        files: [
            "qtcreator/designer/templates/Dialog_with_Buttons_Bottom.ui",
            "qtcreator/designer/templates/Dialog_with_Buttons_Right.ui",
            "qtcreator/designer/templates/Dialog_without_Buttons.ui",
            "qtcreator/designer/templates/Main_Window.ui",
            "qtcreator/designer/templates/Widget.ui",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/dumper"
        fileTags: ["install"]
        files: [
            "qtcreator/dumper/LGPL_EXCEPTION.TXT",
            "qtcreator/dumper/LICENSE.LGPL",
            "qtcreator/dumper/bridge.py",
            "qtcreator/dumper/dumper.cpp",
            "qtcreator/dumper/dumper.h",
            "qtcreator/dumper/dumper.pro",
            "qtcreator/dumper/dumper.py",
            "qtcreator/dumper/dumper_p.h",
            "qtcreator/dumper/pdumper.py",
            "qtcreator/dumper/qttypes.py",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/dumper/test"
        fileTags: ["install"]
        files: [
            "qtcreator/dumper/test/dumpertest.pro",
            "qtcreator/dumper/test/main.cpp",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/generic-highlighter"
        fileTags: ["install"]
        files: [
            "qtcreator/generic-highlighter/alert.xml",
            "qtcreator/generic-highlighter/autoconf.xml",
            "qtcreator/generic-highlighter/bash.xml",
            "qtcreator/generic-highlighter/cmake.xml",
            "qtcreator/generic-highlighter/css.xml",
            "qtcreator/generic-highlighter/doxygen.xml",
            "qtcreator/generic-highlighter/dtd.xml",
            "qtcreator/generic-highlighter/html.xml",
            "qtcreator/generic-highlighter/ini.xml",
            "qtcreator/generic-highlighter/java.xml",
            "qtcreator/generic-highlighter/javadoc.xml",
            "qtcreator/generic-highlighter/perl.xml",
            "qtcreator/generic-highlighter/ruby.xml",
            "qtcreator/generic-highlighter/valgrind-suppression.xml",
            "qtcreator/generic-highlighter/xml.xml",
            "qtcreator/generic-highlighter/yacc.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/glsl"
        fileTags: ["install"]
        files: [
            "qtcreator/glsl/glsl_120.frag",
            "qtcreator/glsl/glsl_120.vert",
            "qtcreator/glsl/glsl_120_common.glsl",
            "qtcreator/glsl/glsl_es_100.frag",
            "qtcreator/glsl/glsl_es_100.vert",
            "qtcreator/glsl/glsl_es_100_common.glsl",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/patches"
        fileTags: ["install"]
        files: [
            "qtcreator/patches/.gitattributes",
            "qtcreator/patches/README",
            "qtcreator/patches/gdb-expand-line-sal-maybe.patch",
            "qtcreator/patches/gdb-increased-dcache-line-size.patch",
            "qtcreator/patches/gdb-stepping-for-maemo.patch",
            "qtcreator/patches/gdb-without-dwarf-name-canonicalization.patch",
            "qtcreator/patches/gdb-work-around-trk-single-step.patch",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml-type-descriptions"
        fileTags: ["install"]
        files: [
            "qtcreator/qml-type-descriptions/builtins.qmltypes",
            "qtcreator/qml-type-descriptions/qmlproject.qmltypes",
            "qtcreator/qml-type-descriptions/qmlruntime.qmltypes",
            "qtcreator/qml-type-descriptions/qt-labs-folderlistmodel.qmltypes",
            "qtcreator/qml-type-descriptions/qt-labs-gestures.qmltypes",
            "qtcreator/qml-type-descriptions/qt-labs-particles.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-connectivity.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-contacts.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-feedback.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-gallery.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-location.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-messaging.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-organizer.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-publishsubscribe.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-sensors.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-serviceframework.qmltypes",
            "qtcreator/qml-type-descriptions/qtmobility-systeminfo.qmltypes",
            "qtcreator/qml-type-descriptions/qtmultimediakit.qmltypes",
            "qtcreator/qml-type-descriptions/qtwebkit.qmltypes",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmldump"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmldump/Info.plist.in",
            "qtcreator/qml/qmldump/LGPL_EXCEPTION.TXT",
            "qtcreator/qml/qmldump/LICENSE.LGPL",
            "qtcreator/qml/qmldump/main.cpp",
            "qtcreator/qml/qmldump/qmldump.pro",
            "qtcreator/qml/qmldump/qmlstreamwriter.cpp",
            "qtcreator/qml/qmldump/qmlstreamwriter.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmljsdebugger/jsdebuggeragent.cpp",
            "qtcreator/qml/qmljsdebugger/qdeclarativeinspectorservice.cpp",
            "qtcreator/qml/qmljsdebugger/qdeclarativeviewinspector.cpp",
            "qtcreator/qml/qmljsdebugger/qdeclarativeviewinspector_p.h",
            "qtcreator/qml/qmljsdebugger/qmljsdebugger-lib.pri",
            "qtcreator/qml/qmljsdebugger/qmljsdebugger-src.pri",
            "qtcreator/qml/qmljsdebugger/qmljsdebugger.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/editor"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmljsdebugger/editor/abstractliveedittool.cpp",
            "qtcreator/qml/qmljsdebugger/editor/abstractliveedittool.h",
            "qtcreator/qml/qmljsdebugger/editor/boundingrecthighlighter.cpp",
            "qtcreator/qml/qmljsdebugger/editor/boundingrecthighlighter.h",
            "qtcreator/qml/qmljsdebugger/editor/colorpickertool.cpp",
            "qtcreator/qml/qmljsdebugger/editor/colorpickertool.h",
            "qtcreator/qml/qmljsdebugger/editor/livelayeritem.cpp",
            "qtcreator/qml/qmljsdebugger/editor/livelayeritem.h",
            "qtcreator/qml/qmljsdebugger/editor/liverubberbandselectionmanipulator.cpp",
            "qtcreator/qml/qmljsdebugger/editor/liverubberbandselectionmanipulator.h",
            "qtcreator/qml/qmljsdebugger/editor/liveselectionindicator.cpp",
            "qtcreator/qml/qmljsdebugger/editor/liveselectionindicator.h",
            "qtcreator/qml/qmljsdebugger/editor/liveselectionrectangle.cpp",
            "qtcreator/qml/qmljsdebugger/editor/liveselectionrectangle.h",
            "qtcreator/qml/qmljsdebugger/editor/liveselectiontool.cpp",
            "qtcreator/qml/qmljsdebugger/editor/liveselectiontool.h",
            "qtcreator/qml/qmljsdebugger/editor/livesingleselectionmanipulator.cpp",
            "qtcreator/qml/qmljsdebugger/editor/livesingleselectionmanipulator.h",
            "qtcreator/qml/qmljsdebugger/editor/subcomponentmasklayeritem.cpp",
            "qtcreator/qml/qmljsdebugger/editor/subcomponentmasklayeritem.h",
            "qtcreator/qml/qmljsdebugger/editor/zoomtool.cpp",
            "qtcreator/qml/qmljsdebugger/editor/zoomtool.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/include"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmljsdebugger/include/jsdebuggeragent.h",
            "qtcreator/qml/qmljsdebugger/include/qdeclarativeinspectorservice.h",
            "qtcreator/qml/qmljsdebugger/include/qdeclarativeviewinspector.h",
            "qtcreator/qml/qmljsdebugger/include/qdeclarativeviewobserver.h",
            "qtcreator/qml/qmljsdebugger/include/qmlinspectorconstants.h",
            "qtcreator/qml/qmljsdebugger/include/qmljsdebugger_global.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/include/qt_private"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmljsdebugger/include/qt_private/qdeclarativedebughelper_p.h",
            "qtcreator/qml/qmljsdebugger/include/qt_private/qdeclarativedebugservice_p.h",
            "qtcreator/qml/qmljsdebugger/include/qt_private/qdeclarativestate_p.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmljsdebugger/protocol"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmljsdebugger/protocol/inspectorprotocol.h",
            "qtcreator/qml/qmljsdebugger/protocol/protocol.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlobserver/Info.plist.in",
            "qtcreator/qml/qmlobserver/LGPL_EXCEPTION.TXT",
            "qtcreator/qml/qmlobserver/LICENSE.LGPL",
            "qtcreator/qml/qmlobserver/deviceorientation.cpp",
            "qtcreator/qml/qmlobserver/deviceorientation.h",
            "qtcreator/qml/qmlobserver/deviceorientation_harmattan.cpp",
            "qtcreator/qml/qmlobserver/deviceorientation_maemo5.cpp",
            "qtcreator/qml/qmlobserver/deviceorientation_symbian.cpp",
            "qtcreator/qml/qmlobserver/loggerwidget.cpp",
            "qtcreator/qml/qmlobserver/loggerwidget.h",
            "qtcreator/qml/qmlobserver/main.cpp",
            "qtcreator/qml/qmlobserver/proxysettings.cpp",
            "qtcreator/qml/qmlobserver/proxysettings.h",
            "qtcreator/qml/qmlobserver/proxysettings.ui",
            "qtcreator/qml/qmlobserver/proxysettings_maemo5.ui",
            "qtcreator/qml/qmlobserver/qdeclarativetester.cpp",
            "qtcreator/qml/qmlobserver/qdeclarativetester.h",
            "qtcreator/qml/qmlobserver/qml.icns",
            "qtcreator/qml/qmlobserver/qml.pri",
            "qtcreator/qml/qmlobserver/qmlobserver.pro",
            "qtcreator/qml/qmlobserver/qmlruntime.cpp",
            "qtcreator/qml/qmlobserver/qmlruntime.h",
            "qtcreator/qml/qmlobserver/recopts.ui",
            "qtcreator/qml/qmlobserver/recopts_maemo5.ui",
            "qtcreator/qml/qmlobserver/texteditautoresizer_maemo5.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver/browser"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlobserver/browser/Browser.qml",
            "qtcreator/qml/qmlobserver/browser/browser.qrc",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver/browser/images"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlobserver/browser/images/folder.png",
            "qtcreator/qml/qmlobserver/browser/images/titlebar.png",
            "qtcreator/qml/qmlobserver/browser/images/titlebar.sci",
            "qtcreator/qml/qmlobserver/browser/images/up.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlobserver/startup"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlobserver/startup/Logo.qml",
            "qtcreator/qml/qmlobserver/startup/qt-back.png",
            "qtcreator/qml/qmlobserver/startup/qt-blue.jpg",
            "qtcreator/qml/qmlobserver/startup/qt-front.png",
            "qtcreator/qml/qmlobserver/startup/qt-sketch.jpg",
            "qtcreator/qml/qmlobserver/startup/qt-text.png",
            "qtcreator/qml/qmlobserver/startup/quick-blur.png",
            "qtcreator/qml/qmlobserver/startup/quick-regular.png",
            "qtcreator/qml/qmlobserver/startup/shadow.png",
            "qtcreator/qml/qmlobserver/startup/startup.qml",
            "qtcreator/qml/qmlobserver/startup/startup.qrc",
            "qtcreator/qml/qmlobserver/startup/white-star.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/qmlpuppet.pro",
            "qtcreator/qml/qmlpuppet/qmlpuppet.qrc",
            "qtcreator/qml/qmlpuppet/qmlpuppet_utilities.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/commands"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/commands/changeauxiliarycommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/changeauxiliarycommand.h",
            "qtcreator/qml/qmlpuppet/commands/changebindingscommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/changebindingscommand.h",
            "qtcreator/qml/qmlpuppet/commands/changefileurlcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/changefileurlcommand.h",
            "qtcreator/qml/qmlpuppet/commands/changeidscommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/changeidscommand.h",
            "qtcreator/qml/qmlpuppet/commands/changenodesourcecommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/changenodesourcecommand.h",
            "qtcreator/qml/qmlpuppet/commands/changestatecommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/changestatecommand.h",
            "qtcreator/qml/qmlpuppet/commands/changevaluescommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/changevaluescommand.h",
            "qtcreator/qml/qmlpuppet/commands/childrenchangedcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/childrenchangedcommand.h",
            "qtcreator/qml/qmlpuppet/commands/clearscenecommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/clearscenecommand.h",
            "qtcreator/qml/qmlpuppet/commands/commands.pri",
            "qtcreator/qml/qmlpuppet/commands/completecomponentcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/completecomponentcommand.h",
            "qtcreator/qml/qmlpuppet/commands/componentcompletedcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/componentcompletedcommand.h",
            "qtcreator/qml/qmlpuppet/commands/createinstancescommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/createinstancescommand.h",
            "qtcreator/qml/qmlpuppet/commands/createscenecommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/createscenecommand.h",
            "qtcreator/qml/qmlpuppet/commands/informationchangedcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/informationchangedcommand.h",
            "qtcreator/qml/qmlpuppet/commands/pixmapchangedcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/pixmapchangedcommand.h",
            "qtcreator/qml/qmlpuppet/commands/removeinstancescommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/removeinstancescommand.h",
            "qtcreator/qml/qmlpuppet/commands/removepropertiescommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/removepropertiescommand.h",
            "qtcreator/qml/qmlpuppet/commands/reparentinstancescommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/reparentinstancescommand.h",
            "qtcreator/qml/qmlpuppet/commands/statepreviewimagechangedcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/statepreviewimagechangedcommand.h",
            "qtcreator/qml/qmlpuppet/commands/synchronizecommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/synchronizecommand.h",
            "qtcreator/qml/qmlpuppet/commands/tokencommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/tokencommand.h",
            "qtcreator/qml/qmlpuppet/commands/valueschangedcommand.cpp",
            "qtcreator/qml/qmlpuppet/commands/valueschangedcommand.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/container"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/container/addimportcontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/addimportcontainer.h",
            "qtcreator/qml/qmlpuppet/container/container.pri",
            "qtcreator/qml/qmlpuppet/container/idcontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/idcontainer.h",
            "qtcreator/qml/qmlpuppet/container/imagecontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/imagecontainer.h",
            "qtcreator/qml/qmlpuppet/container/informationcontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/informationcontainer.h",
            "qtcreator/qml/qmlpuppet/container/instancecontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/instancecontainer.h",
            "qtcreator/qml/qmlpuppet/container/propertyabstractcontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/propertyabstractcontainer.h",
            "qtcreator/qml/qmlpuppet/container/propertybindingcontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/propertybindingcontainer.h",
            "qtcreator/qml/qmlpuppet/container/propertyvaluecontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/propertyvaluecontainer.h",
            "qtcreator/qml/qmlpuppet/container/reparentcontainer.cpp",
            "qtcreator/qml/qmlpuppet/container/reparentcontainer.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/html"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/html/welcome.html",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/images"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/images/template_image.png",
            "qtcreator/qml/qmlpuppet/images/webkit.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/instances"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/instances/anchorchangesnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/anchorchangesnodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/behaviornodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/behaviornodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/childrenchangeeventfilter.cpp",
            "qtcreator/qml/qmlpuppet/instances/childrenchangeeventfilter.h",
            "qtcreator/qml/qmlpuppet/instances/componentnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/componentnodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/dummycontextobject.cpp",
            "qtcreator/qml/qmlpuppet/instances/dummycontextobject.h",
            "qtcreator/qml/qmlpuppet/instances/dummynodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/dummynodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/instances.pri",
            "qtcreator/qml/qmlpuppet/instances/nodeinstanceclientproxy.cpp",
            "qtcreator/qml/qmlpuppet/instances/nodeinstanceclientproxy.h",
            "qtcreator/qml/qmlpuppet/instances/nodeinstancemetaobject.cpp",
            "qtcreator/qml/qmlpuppet/instances/nodeinstancemetaobject.h",
            "qtcreator/qml/qmlpuppet/instances/nodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/instances/nodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/instances/nodeinstancesignalspy.cpp",
            "qtcreator/qml/qmlpuppet/instances/nodeinstancesignalspy.h",
            "qtcreator/qml/qmlpuppet/instances/objectnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/objectnodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/qmlpropertychangesnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/qmlpropertychangesnodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/qmlstatenodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/qmlstatenodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/qmltransitionnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/qmltransitionnodeinstance.h",
            "qtcreator/qml/qmlpuppet/instances/servernodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/instances/servernodeinstance.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/interfaces"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/interfaces/commondefines.h",
            "qtcreator/qml/qmlpuppet/interfaces/interfaces.pri",
            "qtcreator/qml/qmlpuppet/interfaces/nodeinstanceclientinterface.h",
            "qtcreator/qml/qmlpuppet/interfaces/nodeinstanceserverinterface.cpp",
            "qtcreator/qml/qmlpuppet/interfaces/nodeinstanceserverinterface.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qml2puppet"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/qml2puppet/Info.plist.in",
            "qtcreator/qml/qmlpuppet/qml2puppet/main.cpp",
            "qtcreator/qml/qmlpuppet/qml2puppet/qml2puppet.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qml2puppet/instances"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/instances.pri",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5informationnodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5informationnodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5nodeinstanceclientproxy.cpp",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5nodeinstanceclientproxy.h",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5nodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5nodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5previewnodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5previewnodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5rendernodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/qt5rendernodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/sgitemnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/qml2puppet/instances/sgitemnodeinstance.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qmlpuppet"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/qmlpuppet/Info.plist.in",
            "qtcreator/qml/qmlpuppet/qmlpuppet/main.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/qmlpuppet.pri",
            "qtcreator/qml/qmlpuppet/qmlpuppet/qmlpuppet.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qml/qmlpuppet/qmlpuppet/instances"
        fileTags: ["install"]
        files: [
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/graphicsobjectnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/graphicsobjectnodeinstance.h",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/instances.pri",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/positionernodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/positionernodeinstance.h",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qmlgraphicsitemnodeinstance.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qmlgraphicsitemnodeinstance.h",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4informationnodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4informationnodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4nodeinstanceclientproxy.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4nodeinstanceclientproxy.h",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4nodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4nodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4previewnodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4previewnodeinstanceserver.h",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4rendernodeinstanceserver.cpp",
            "qtcreator/qml/qmlpuppet/qmlpuppet/instances/qt4rendernodeinstanceserver.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmldesigner/propertyeditor/Qt"
        fileTags: ["install"]
        files: [
            "qtcreator/qmldesigner/propertyeditor/Qt/AlignmentHorizontalButtons.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/AlignmentVerticalButtons.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/AnchorBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/AnchorButtons.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/BorderImageSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/CheckBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ColorGroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ColorLabel.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ColorScheme.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ColorTypeButtons.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ComboBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/DoubleSpinBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/DoubleSpinBoxAlternate.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ExpressionEditor.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Extended.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ExtendedFunctionButton.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ExtendedPane.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ExtendedSwitches.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FlagedButton.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FlickableGroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FlickableSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FlipableSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FlowSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FontComboBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FontGroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/FontStyleButtons.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Geometry.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/GridSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/GridViewSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/GroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/GroupBoxOption.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/HorizontalLayout.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/HorizontalWhiteLine.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ImageSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/IntEditor.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ItemPane.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Label.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Layout.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/LayoutPane.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/LineEdit.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ListViewSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Modifiers.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/MouseAreaSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/PathViewSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/PlaceHolder.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/PropertyFrame.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/RectangleColorGroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/RectangleSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/RowSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/ScrollArea.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/SliderWidget.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/SpinBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/StandardTextColorGroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/StandardTextGroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Switches.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/TextEditSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/TextEditor.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/TextInputGroupBox.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/TextInputSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/TextSpecifics.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Transformation.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Type.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/VerticalLayout.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/Visibility.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorbottom.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorbox.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorfill.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorhorizontal.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorleft.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorright.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorspacer.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchortop.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/anchorvertical.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/applybutton.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/aspectlock.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/cancelbutton.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/checkbox_tr.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/emptyPane.qml",
            "qtcreator/qmldesigner/propertyeditor/Qt/layoutWidget.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/propertyEditor.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/specialCheckBox.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/styledbuttonleft.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/styledbuttonmiddle.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/styledbuttonright.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/switch.css",
            "qtcreator/qmldesigner/propertyeditor/Qt/typeLabel.css",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmldesigner/propertyeditor/Qt/images"
        fileTags: ["install"]
        files: [
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentbottom-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentbottom-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentcenterh-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentcenterh-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentleft-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentleft-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentmiddle-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentmiddle-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentright-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmentright-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmenttop-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/alignmenttop-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/apply.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/behaivour.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/blended-image-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/bold-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/bold-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/button.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/cancel.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/default-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/downArrow.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/expression.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/extended.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/grid-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/icon_color_gradient.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/icon_color_none.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/icon_color_solid.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/image-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/italic-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/italic-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/item-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/layout.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/leftArrow.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/list-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/mouse-area-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/placeholder.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/rect-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/reset-button.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/rightArrow.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/standard.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/strikeout-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/strikeout-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/submenu.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/text-edit-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/text-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/underline-h-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/underline-icon.png",
            "qtcreator/qmldesigner/propertyeditor/Qt/images/upArrow.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmldesigner/propertyeditor/QtWebKit"
        fileTags: ["install"]
        files: [
            "qtcreator/qmldesigner/propertyeditor/QtWebKit/WebViewSpecifics.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmlicons/Qt/16x16"
        fileTags: ["install"]
        files: [
            "qtcreator/qmlicons/Qt/16x16/BorderImage.png",
            "qtcreator/qmlicons/Qt/16x16/BusyIndicator.png",
            "qtcreator/qmlicons/Qt/16x16/Button.png",
            "qtcreator/qmlicons/Qt/16x16/ButtonColumn.png",
            "qtcreator/qmlicons/Qt/16x16/ButtonRow.png",
            "qtcreator/qmlicons/Qt/16x16/CheckBox.png",
            "qtcreator/qmlicons/Qt/16x16/ChoiceList.png",
            "qtcreator/qmlicons/Qt/16x16/ColorAnimation.png",
            "qtcreator/qmlicons/Qt/16x16/Component.png",
            "qtcreator/qmlicons/Qt/16x16/CountBubble.png",
            "qtcreator/qmlicons/Qt/16x16/DatePickerDialog.png",
            "qtcreator/qmlicons/Qt/16x16/Flickable.png",
            "qtcreator/qmlicons/Qt/16x16/Flipable.png",
            "qtcreator/qmlicons/Qt/16x16/FocusScope.png",
            "qtcreator/qmlicons/Qt/16x16/GridView.png",
            "qtcreator/qmlicons/Qt/16x16/Image.png",
            "qtcreator/qmlicons/Qt/16x16/InfoBanner.png",
            "qtcreator/qmlicons/Qt/16x16/Item.png",
            "qtcreator/qmlicons/Qt/16x16/ListButton.png",
            "qtcreator/qmlicons/Qt/16x16/ListDelegate.png",
            "qtcreator/qmlicons/Qt/16x16/ListView.png",
            "qtcreator/qmlicons/Qt/16x16/MoreIndicator.png",
            "qtcreator/qmlicons/Qt/16x16/MouseArea.png",
            "qtcreator/qmlicons/Qt/16x16/PageIndicator.png",
            "qtcreator/qmlicons/Qt/16x16/ParallelAnimation.png",
            "qtcreator/qmlicons/Qt/16x16/PathView.png",
            "qtcreator/qmlicons/Qt/16x16/PauseAnimation.png",
            "qtcreator/qmlicons/Qt/16x16/ProgressBar.png",
            "qtcreator/qmlicons/Qt/16x16/PropertyChanges.png",
            "qtcreator/qmlicons/Qt/16x16/RadioButton.png",
            "qtcreator/qmlicons/Qt/16x16/RatingIndicator.png",
            "qtcreator/qmlicons/Qt/16x16/Rectangle.png",
            "qtcreator/qmlicons/Qt/16x16/SequentialAnimation.png",
            "qtcreator/qmlicons/Qt/16x16/Slider.png",
            "qtcreator/qmlicons/Qt/16x16/State.png",
            "qtcreator/qmlicons/Qt/16x16/Switch.png",
            "qtcreator/qmlicons/Qt/16x16/TabBar.png",
            "qtcreator/qmlicons/Qt/16x16/TabButton.png",
            "qtcreator/qmlicons/Qt/16x16/Text.png",
            "qtcreator/qmlicons/Qt/16x16/TextArea.png",
            "qtcreator/qmlicons/Qt/16x16/TextEdit.png",
            "qtcreator/qmlicons/Qt/16x16/TextField.png",
            "qtcreator/qmlicons/Qt/16x16/TextInput.png",
            "qtcreator/qmlicons/Qt/16x16/TimePickerDialog.png",
            "qtcreator/qmlicons/Qt/16x16/ToolBar.png",
            "qtcreator/qmlicons/Qt/16x16/Transition.png",
            "qtcreator/qmlicons/Qt/16x16/Tumbler.png",
            "qtcreator/qmlicons/Qt/16x16/TumblerButton.png",
            "qtcreator/qmlicons/Qt/16x16/TumblerColumn.png",
            "qtcreator/qmlicons/Qt/16x16/TumblerDialog.png",
            "qtcreator/qmlicons/Qt/16x16/Window.png",
            "qtcreator/qmlicons/Qt/16x16/item-icon16.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/qmlicons/QtWebkit/16x16"
        fileTags: ["install"]
        files: [
            "qtcreator/qmlicons/QtWebkit/16x16/WebView.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/schemes"
        fileTags: ["install"]
        files: [
            "qtcreator/schemes/MS_Visual_C++.kms",
            "qtcreator/schemes/Xcode.kms",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/scripts"
        fileTags: ["install"]
        files: [
            "qtcreator/scripts/openTerminal.command",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/snippets"
        fileTags: ["install"]
        files: [
            "qtcreator/snippets/cpp.xml",
            "qtcreator/snippets/qml.xml",
            "qtcreator/snippets/text.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/styles"
        fileTags: ["install"]
        files: [
            "qtcreator/styles/darkvim.xml",
            "qtcreator/styles/default.xml",
            "qtcreator/styles/grayscale.xml",
            "qtcreator/styles/inkpot.xml",
            "qtcreator/styles/intellij.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/html5app/app.pro",
            "qtcreator/templates/html5app/main.cpp",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app/html"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/html5app/html/index.html",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app/html5applicationviewer"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/html5app/html5applicationviewer/html5applicationviewer.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/html5applicationviewer.h",
            "qtcreator/templates/html5app/html5applicationviewer/html5applicationviewer.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/html5app/html5applicationviewer/touchnavigation"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/navigationcontroller.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/navigationcontroller.h",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/touchnavigation.pri",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webnavigation.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webnavigation.h",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchevent.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchevent.h",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchnavigation.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchnavigation.h",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchphysics.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchphysics.h",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchphysicsinterface.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchphysicsinterface.h",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchscroller.cpp",
            "qtcreator/templates/html5app/html5applicationviewer/touchnavigation/webtouchscroller.h",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/mobileapp"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/mobileapp/app.pro",
            "qtcreator/templates/mobileapp/main.cpp",
            "qtcreator/templates/mobileapp/mainwindow.cpp",
            "qtcreator/templates/mobileapp/mainwindow.h",
            "qtcreator/templates/mobileapp/mainwindow.ui",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qt4project"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/qt4project/main.cpp",
            "qtcreator/templates/qt4project/mywidget.cpp",
            "qtcreator/templates/qt4project/mywidget.h",
            "qtcreator/templates/qt4project/mywidget_form.cpp",
            "qtcreator/templates/qt4project/mywidget_form.h",
            "qtcreator/templates/qt4project/widget.ui",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qt4project/customwidgetwizard"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_collection.cpp",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_collection.h",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_plugin.pro",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_resources.qrc",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_single.cpp",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_single.h",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_widget.cpp",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_widget.h",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_widget_include.pri",
            "qtcreator/templates/qt4project/customwidgetwizard/tpl_widget_lib.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/qtquickapp/app.pro",
            "qtcreator/templates/qtquickapp/main.cpp",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp/qml/app/meego10"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/qtquickapp/qml/app/meego10/MainPage.qml",
            "qtcreator/templates/qtquickapp/qml/app/meego10/main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp/qml/app/qtquick10"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/qtquickapp/qml/app/qtquick10/main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp/qml/app/symbian11"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/qtquickapp/qml/app/symbian11/MainPage.qml",
            "qtcreator/templates/qtquickapp/qml/app/symbian11/main.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/qtquickapp/qmlapplicationviewer"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/qtquickapp/qmlapplicationviewer/qmlapplicationviewer.cpp",
            "qtcreator/templates/qtquickapp/qmlapplicationviewer/qmlapplicationviewer.h",
            "qtcreator/templates/qtquickapp/qmlapplicationviewer/qmlapplicationviewer.pri",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/shared"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/shared/app.desktop",
            "qtcreator/templates/shared/deployment.pri",
            "qtcreator/templates/shared/icon64.png",
            "qtcreator/templates/shared/icon80.png",
            "qtcreator/templates/shared/manifest.aegis",
            "qtcreator/templates/shared/symbianicon.svg",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/README.txt",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/helloworld"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/helloworld/console.png",
            "qtcreator/templates/wizards/helloworld/main.cpp",
            "qtcreator/templates/wizards/helloworld/project.pro",
            "qtcreator/templates/wizards/helloworld/wizard_sample.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/listmodel"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/listmodel/listmodel.cpp",
            "qtcreator/templates/wizards/listmodel/listmodel.h",
            "qtcreator/templates/wizards/listmodel/wizard_sample.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincapp"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/plaincapp/console.png",
            "qtcreator/templates/wizards/plaincapp/main.c",
            "qtcreator/templates/wizards/plaincapp/project.pro",
            "qtcreator/templates/wizards/plaincapp/wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincapp-cmake"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/plaincapp-cmake/CMakeLists.txt",
            "qtcreator/templates/wizards/plaincapp-cmake/console.png",
            "qtcreator/templates/wizards/plaincapp-cmake/main.c",
            "qtcreator/templates/wizards/plaincapp-cmake/wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincppapp"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/plaincppapp/console.png",
            "qtcreator/templates/wizards/plaincppapp/main.cpp",
            "qtcreator/templates/wizards/plaincppapp/project.pro",
            "qtcreator/templates/wizards/plaincppapp/wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/plaincppapp-cmake"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/plaincppapp-cmake/CMakeLists.txt",
            "qtcreator/templates/wizards/plaincppapp-cmake/console.png",
            "qtcreator/templates/wizards/plaincppapp-cmake/main.cpp",
            "qtcreator/templates/wizards/plaincppapp-cmake/wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/qml-extension"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/qml-extension/lib.png",
            "qtcreator/templates/wizards/qml-extension/object.cpp",
            "qtcreator/templates/wizards/qml-extension/object.h",
            "qtcreator/templates/wizards/qml-extension/plugin.cpp",
            "qtcreator/templates/wizards/qml-extension/plugin.h",
            "qtcreator/templates/wizards/qml-extension/project.pro",
            "qtcreator/templates/wizards/qml-extension/qmldir",
            "qtcreator/templates/wizards/qml-extension/wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/qtcreatorplugin"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/qtcreatorplugin/MyPlugin.pluginspec.in",
            "qtcreator/templates/wizards/qtcreatorplugin/myplugin.cpp",
            "qtcreator/templates/wizards/qtcreatorplugin/myplugin.h",
            "qtcreator/templates/wizards/qtcreatorplugin/myplugin.pro",
            "qtcreator/templates/wizards/qtcreatorplugin/myplugin_global.h",
            "qtcreator/templates/wizards/qtcreatorplugin/mypluginconstants.h",
            "qtcreator/templates/wizards/qtcreatorplugin/qtcreator_logo_24.png",
            "qtcreator/templates/wizards/qtcreatorplugin/wizard.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/templates/wizards/scriptgeneratedproject"
        fileTags: ["install"]
        files: [
            "qtcreator/templates/wizards/scriptgeneratedproject/generate.pl",
            "qtcreator/templates/wizards/scriptgeneratedproject/wizard_sample.xml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/translations"
        fileTags: ["install"]
        files: [
            "qtcreator/translations/README",
            "qtcreator/translations/check-ts.pl",
            "qtcreator/translations/check-ts.xq",
            "qtcreator/translations/extract-customwizards.xq",
            "qtcreator/translations/extract-externaltools.xq",
            "qtcreator/translations/extract-mimetypes.xq",
            "qtcreator/translations/qtcreator_cs.ts",
            "qtcreator/translations/qtcreator_de.ts",
            "qtcreator/translations/qtcreator_es.ts",
            "qtcreator/translations/qtcreator_fr.ts",
            "qtcreator/translations/qtcreator_hu.ts",
            "qtcreator/translations/qtcreator_it.ts",
            "qtcreator/translations/qtcreator_ja.ts",
            "qtcreator/translations/qtcreator_pl.ts",
            "qtcreator/translations/qtcreator_ru.ts",
            "qtcreator/translations/qtcreator_sl.ts",
            "qtcreator/translations/qtcreator_uk.ts",
            "qtcreator/translations/qtcreator_zh_CN.ts",
            "qtcreator/translations/translations.pro",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/develop.qml",
            "qtcreator/welcomescreen/examples.qml",
            "qtcreator/welcomescreen/examples_fallback.xml",
            "qtcreator/welcomescreen/gettingstarted.qml",
            "qtcreator/welcomescreen/images_areaofinterest.xml",
            "qtcreator/welcomescreen/qtcreator_tutorials.xml",
            "qtcreator/welcomescreen/tutorials.qml",
            "qtcreator/welcomescreen/welcomescreen.qml",
            "qtcreator/welcomescreen/welcomescreen.qmlproject",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/dummydata"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/dummydata/examplesModel.qml",
            "qtcreator/welcomescreen/dummydata/pagesModel.qml",
            "qtcreator/welcomescreen/dummydata/projectList.qml",
            "qtcreator/welcomescreen/dummydata/sessionList.qml",
            "qtcreator/welcomescreen/dummydata/tutorialsModel.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/widgets/CustomColors.qml",
            "qtcreator/welcomescreen/widgets/CustomFonts.qml",
            "qtcreator/welcomescreen/widgets/CustomTab.qml",
            "qtcreator/welcomescreen/widgets/CustomizedGridView.qml",
            "qtcreator/welcomescreen/widgets/Delegate.qml",
            "qtcreator/welcomescreen/widgets/Feedback.qml",
            "qtcreator/welcomescreen/widgets/GettingStartedItem.qml",
            "qtcreator/welcomescreen/widgets/IconAndLink.qml",
            "qtcreator/welcomescreen/widgets/LinkedText.qml",
            "qtcreator/welcomescreen/widgets/LinksBar.qml",
            "qtcreator/welcomescreen/widgets/Logo.qml",
            "qtcreator/welcomescreen/widgets/PageCaption.qml",
            "qtcreator/welcomescreen/widgets/PageLoader.qml",
            "qtcreator/welcomescreen/widgets/ProjectItem.qml",
            "qtcreator/welcomescreen/widgets/RecentProjects.qml",
            "qtcreator/welcomescreen/widgets/SearchBar.qml",
            "qtcreator/welcomescreen/widgets/SessionItem.qml",
            "qtcreator/welcomescreen/widgets/Sessions.qml",
            "qtcreator/welcomescreen/widgets/ToolTip.qml",
            "qtcreator/welcomescreen/widgets/qmldir",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/dummydata"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/widgets/dummydata/examplesModel.qml",
            "qtcreator/welcomescreen/widgets/dummydata/mockupTags.qml",
            "qtcreator/welcomescreen/widgets/dummydata/pagesModel.qml",
            "qtcreator/welcomescreen/widgets/dummydata/tabsModel.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/dummydata/context"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/widgets/dummydata/context/ExampleDelegate.qml",
            "qtcreator/welcomescreen/widgets/dummydata/context/ExampleGridView.qml",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/images"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/widgets/images/arrowBig.png",
            "qtcreator/welcomescreen/widgets/images/arrow_down.png",
            "qtcreator/welcomescreen/widgets/images/arrow_up.png",
            "qtcreator/welcomescreen/widgets/images/bullet.png",
            "qtcreator/welcomescreen/widgets/images/dropshadow.png",
            "qtcreator/welcomescreen/widgets/images/gettingStarted01.png",
            "qtcreator/welcomescreen/widgets/images/gettingStarted02.png",
            "qtcreator/welcomescreen/widgets/images/gettingStarted03.png",
            "qtcreator/welcomescreen/widgets/images/gettingStarted04.png",
            "qtcreator/welcomescreen/widgets/images/info.png",
            "qtcreator/welcomescreen/widgets/images/more.png",
            "qtcreator/welcomescreen/widgets/images/qtcreator.png",
            "qtcreator/welcomescreen/widgets/images/tab.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/images/icons"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/widgets/images/icons/adressbook.png",
            "qtcreator/welcomescreen/widgets/images/icons/buildrun.png",
            "qtcreator/welcomescreen/widgets/images/icons/clone.png",
            "qtcreator/welcomescreen/widgets/images/icons/communityIcon.png",
            "qtcreator/welcomescreen/widgets/images/icons/components.png",
            "qtcreator/welcomescreen/widgets/images/icons/createIcon.png",
            "qtcreator/welcomescreen/widgets/images/icons/ddays09.png",
            "qtcreator/welcomescreen/widgets/images/icons/ddays10.png",
            "qtcreator/welcomescreen/widgets/images/icons/ddays11.png",
            "qtcreator/welcomescreen/widgets/images/icons/delete.png",
            "qtcreator/welcomescreen/widgets/images/icons/developing_with_qt_creator.png",
            "qtcreator/welcomescreen/widgets/images/icons/feedbackIcon.png",
            "qtcreator/welcomescreen/widgets/images/icons/ico_community.png",
            "qtcreator/welcomescreen/widgets/images/icons/labsIcon.png",
            "qtcreator/welcomescreen/widgets/images/icons/openIcon.png",
            "qtcreator/welcomescreen/widgets/images/icons/qt_quick_1.png",
            "qtcreator/welcomescreen/widgets/images/icons/qt_quick_2.png",
            "qtcreator/welcomescreen/widgets/images/icons/qt_quick_3.png",
            "qtcreator/welcomescreen/widgets/images/icons/qt_sdk.png",
            "qtcreator/welcomescreen/widgets/images/icons/qtquick.png",
            "qtcreator/welcomescreen/widgets/images/icons/qwidget.png",
            "qtcreator/welcomescreen/widgets/images/icons/rename.png",
            "qtcreator/welcomescreen/widgets/images/icons/userguideIcon.png",
            "qtcreator/welcomescreen/widgets/images/icons/videoIcon.png",
        ]
    }

    Group {
        qbs.installDir: "share/qtcreator/welcomescreen/widgets/images/mockup"
        fileTags: ["install"]
        files: [
            "qtcreator/welcomescreen/widgets/images/mockup/designer-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/desktop-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/draganddrop-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/itemview-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/layout-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/mainwindow-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/network-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/opengl-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/penguin.png",
            "qtcreator/welcomescreen/widgets/images/mockup/qtscript-examples.png",
            "qtcreator/welcomescreen/widgets/images/mockup/thread-examples.png",
        ]
    }
}

