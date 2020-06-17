import qbs.Utilities
import QtcFunctions

QtcTool {
    name: "qml2puppet"
    installDir: qbs.targetOS.contains("macos")
                ? qtc.ide_libexec_path + "/qmldesigner" : qtc.ide_libexec_path

    Depends { name: "Utils" }
    Depends { name: "bundle" }
    Depends {
        name: "Qt"
        submodules: [
            "core-private", "gui-private", "network", "qml-private", "quick", "quick-private",
            "widgets"
        ]
    }
    Depends { name: "Qt.quick3d-private"; required: false }
    property bool useQuick3d: Utilities.versionCompare(Qt.core.version, "5.15") >= 0
                              && Qt["quick3d-private"].present

    cpp.defines: {
        var defines = base.filter(function(d) { return d != "QT_CREATOR"; });
        if (useQuick3d)
            defines.push("QUICK3D_MODULE");
        return defines;
    }
    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("bsd")
        cpp.dynamicLibraries: base.concat("rt")
    }
    Properties {
        condition: qbs.targetOS.contains("darwin")
        bundle.embedInfoPlist: true
    }

    property path puppetDir: "../../../share/qtcreator/qml/qmlpuppet"
    cpp.includePaths: base.concat([
        puppetDir + "/commands",
        puppetDir + "/container",
        puppetDir + "/instances",
        puppetDir + "/interfaces",
        puppetDir + "/types",
        puppetDir + "/qml2puppet/instances",
        puppetDir + "/qmlprivategate",
    ])

    // TODO: Put into static lib and share with qmldesigner
    Group {
        name: "puppet sources"
        prefix: puppetDir + '/'
        files: [
            "commands/changeauxiliarycommand.cpp",
            "commands/changeauxiliarycommand.h",
            "commands/changebindingscommand.cpp",
            "commands/changebindingscommand.h",
            "commands/changefileurlcommand.cpp",
            "commands/changefileurlcommand.h",
            "commands/changeidscommand.cpp",
            "commands/changeidscommand.h",
            "commands/changenodesourcecommand.cpp",
            "commands/changenodesourcecommand.h",
            "commands/changestatecommand.cpp",
            "commands/changestatecommand.h",
            "commands/changevaluescommand.cpp",
            "commands/changevaluescommand.h",
            "commands/childrenchangedcommand.cpp",
            "commands/childrenchangedcommand.h",
            "commands/clearscenecommand.cpp",
            "commands/clearscenecommand.h",
            "commands/completecomponentcommand.cpp",
            "commands/completecomponentcommand.h",
            "commands/componentcompletedcommand.cpp",
            "commands/componentcompletedcommand.h",
            "commands/createinstancescommand.cpp",
            "commands/createinstancescommand.h",
            "commands/createscenecommand.cpp",
            "commands/createscenecommand.h",
            "commands/debugoutputcommand.cpp",
            "commands/debugoutputcommand.h",
            "commands/endpuppetcommand.cpp",
            "commands/endpuppetcommand.h",
            "commands/informationchangedcommand.cpp",
            "commands/informationchangedcommand.h",
            "commands/pixmapchangedcommand.cpp",
            "commands/pixmapchangedcommand.h",
            "commands/puppetalivecommand.cpp",
            "commands/puppetalivecommand.h",
            "commands/removeinstancescommand.cpp",
            "commands/removeinstancescommand.h",
            "commands/removepropertiescommand.cpp",
            "commands/removepropertiescommand.h",
            "commands/removesharedmemorycommand.cpp",
            "commands/removesharedmemorycommand.h",
            "commands/reparentinstancescommand.cpp",
            "commands/reparentinstancescommand.h",
            "commands/statepreviewimagechangedcommand.cpp",
            "commands/statepreviewimagechangedcommand.h",
            "commands/synchronizecommand.cpp",
            "commands/synchronizecommand.h",
            "commands/changepreviewimagesizecommand.cpp",
            "commands/changepreviewimagesizecommand.h",
            "commands/changelanguagecommand.cpp",
            "commands/changelanguagecommand.h",
            "commands/tokencommand.cpp",
            "commands/tokencommand.h",
            "commands/valueschangedcommand.cpp",
            "commands/valueschangedcommand.h",
            "commands/changeselectioncommand.cpp",
            "commands/changeselectioncommand.h",
            "commands/update3dviewstatecommand.cpp",
            "commands/update3dviewstatecommand.h",
            "commands/puppettocreatorcommand.cpp",
            "commands/puppettocreatorcommand.h",
            "commands/inputeventcommand.cpp",
            "commands/inputeventcommand.h",
            "commands/view3dactioncommand.cpp",
            "commands/view3dactioncommand.h",
            "container/addimportcontainer.cpp",
            "container/addimportcontainer.h",
            "container/idcontainer.cpp",
            "container/idcontainer.h",
            "container/imagecontainer.cpp",
            "container/imagecontainer.h",
            "container/informationcontainer.cpp",
            "container/informationcontainer.h",
            "container/instancecontainer.cpp",
            "container/instancecontainer.h",
            "container/mockuptypecontainer.cpp",
            "container/mockuptypecontainer.h",
            "container/propertyabstractcontainer.cpp",
            "container/propertyabstractcontainer.h",
            "container/propertybindingcontainer.cpp",
            "container/propertybindingcontainer.h",
            "container/propertyvaluecontainer.cpp",
            "container/propertyvaluecontainer.h",
            "container/reparentcontainer.cpp",
            "container/reparentcontainer.h",
            "container/sharedmemory.h",
            "instances/nodeinstanceclientproxy.cpp",
            "instances/nodeinstanceclientproxy.h",
            "interfaces/commondefines.h",
            "interfaces/nodeinstanceclientinterface.h",
            "interfaces/nodeinstanceserverinterface.cpp",
            "interfaces/nodeinstanceserverinterface.h",
            "qmlprivategate/qmlprivategate.h",
            "types/enumeration.h",
        ]
    }

    Group {
        name: "SharedMemory (Unix)"
        condition: qbs.targetOS.contains("unix")
        prefix: puppetDir + "/container/"
        files: ["sharedmemory_unix.cpp"]
    }

    Group {
        name: "SharedMemory (Generic)"
        condition: !qbs.targetOS.contains("unix")
        prefix: puppetDir + "/container/"
        files: ["sharedmemory_qt.cpp"]
    }

    Group {
        name: "puppet2 sources"
        prefix: puppetDir + "/qml2puppet/"
        files: [
            "../qmlpuppet.qrc",
            "Info.plist",
            "instances/anchorchangesnodeinstance.cpp",
            "instances/anchorchangesnodeinstance.h",
            "instances/behaviornodeinstance.cpp",
            "instances/behaviornodeinstance.h",
            "instances/childrenchangeeventfilter.cpp",
            "instances/childrenchangeeventfilter.h",
            "instances/componentnodeinstance.cpp",
            "instances/componentnodeinstance.h",
            "instances/dummycontextobject.cpp",
            "instances/dummycontextobject.h",
            "instances/dummynodeinstance.cpp",
            "instances/dummynodeinstance.h",
            "instances/layoutnodeinstance.cpp",
            "instances/layoutnodeinstance.h",
            "instances/nodeinstanceserver.cpp",
            "instances/nodeinstanceserver.h",
            "instances/nodeinstancesignalspy.cpp",
            "instances/nodeinstancesignalspy.h",
            "instances/objectnodeinstance.cpp",
            "instances/objectnodeinstance.h",
            "instances/positionernodeinstance.cpp",
            "instances/positionernodeinstance.h",
            "instances/quickitemnodeinstance.cpp",
            "instances/quickitemnodeinstance.h",
            "instances/qmlpropertychangesnodeinstance.cpp",
            "instances/qmlpropertychangesnodeinstance.h",
            "instances/qmlstatenodeinstance.cpp",
            "instances/qmlstatenodeinstance.h",
            "instances/quick3dnodeinstance.cpp",
            "instances/quick3dnodeinstance.h",
            "instances/quick3dtexturenodeinstance.cpp",
            "instances/quick3dtexturenodeinstance.h",
            "instances/qmltransitionnodeinstance.cpp",
            "instances/qmltransitionnodeinstance.h",
            "instances/qt3dpresentationnodeinstance.cpp",
            "instances/qt3dpresentationnodeinstance.h",
            "instances/qt5informationnodeinstanceserver.cpp",
            "instances/qt5informationnodeinstanceserver.h",
            "instances/qt5nodeinstanceclientproxy.cpp",
            "instances/qt5nodeinstanceclientproxy.h",
            "instances/qt5nodeinstanceserver.cpp",
            "instances/qt5nodeinstanceserver.h",
            "instances/qt5previewnodeinstanceserver.cpp",
            "instances/qt5previewnodeinstanceserver.h",
            "instances/qt5rendernodeinstanceserver.cpp",
            "instances/qt5rendernodeinstanceserver.h",
            "instances/qt5testnodeinstanceserver.cpp",
            "instances/qt5testnodeinstanceserver.h",
            "instances/servernodeinstance.cpp",
            "instances/servernodeinstance.h",
            "editor3d/generalhelper.cpp",
            "editor3d/mousearea3d.cpp",
            "editor3d/camerageometry.cpp",
            "editor3d/lightgeometry.cpp",
            "editor3d/gridgeometry.cpp",
            "editor3d/selectionboxgeometry.cpp",
            "editor3d/linegeometry.cpp",
            "editor3d/icongizmoimageprovider.cpp",
            "editor3d/icongizmoimageprovider.h",
            "iconrenderer/iconrenderer.cpp",
            "iconrenderer/iconrenderer.h",
            "qml2puppetmain.cpp",
        ]

        Group {
            name: "3d-only puppet2 headers"
            files: [
                "editor3d/camerageometry.h",
                "editor3d/generalhelper.h",
                "editor3d/gridgeometry.h",
                "editor3d/lightgeometry.h",
                "editor3d/linegeometry.h",
                "editor3d/selectionboxgeometry.h",
                "editor3d/mousearea3d.h",
            ]
            fileTags: product.useQuick3d ? [] : ["unmocable"]
            overrideTags: false
        }
    }

    Group {
        name: "qmlprivategate (new)"
        condition: QtcFunctions.versionIsAtLeast(Qt.core.version, "5.6.0")
        prefix: puppetDir + "/qmlprivategate/"
        files: [
            "qmlprivategate_56.cpp",
        ]
    }

    Group {
        name: "qmlprivategate (old)"
        condition: !QtcFunctions.versionIsAtLeast(Qt.core.version, "5.6.0")
        prefix: puppetDir + "/qmlprivategate/"
        files: [
            "designercustomobjectdata.cpp",
            "designercustomobjectdata.h",
            "metaobject.cpp",
            "metaobject.h",
            "qmlprivategate.cpp",
        ]
    }
}
