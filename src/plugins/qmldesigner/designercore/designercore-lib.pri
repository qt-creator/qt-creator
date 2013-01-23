include($$PWD/filemanager/filemanager.pri)
include (../config.pri)

QT += script \
    network

DEFINES += TEST_EXPORTS
DEFINES += DESIGNER_CORE_LIBRARY
INCLUDEPATH += $$PWD \
    $$PWD/include

include (instances/instances.pri)
include (../../../../share/qtcreator/qml/qmlpuppet/interfaces/interfaces.pri)
include (../../../../share/qtcreator/qml/qmlpuppet/commands/commands.pri)
include (../../../../share/qtcreator/qml/qmlpuppet/container/container.pri)

SOURCES += $$PWD/model/abstractview.cpp \
    $$PWD/model/rewriterview.cpp \
    $$PWD/metainfo/metainfo.cpp \
    $$PWD/metainfo/metainforeader.cpp \
    $$PWD/metainfo/nodemetainfo.cpp \
    $$PWD/metainfo/itemlibraryinfo.cpp \
    $$PWD/metainfo/subcomponentmanager.cpp \
    $$PWD/model/internalproperty.cpp \
    $$PWD/model/model.cpp \
    $$PWD/model/modelnode.cpp \
    $$PWD/model/painteventfilter.cpp \
    $$PWD/model/internalnode.cpp \
    $$PWD/model/propertyparser.cpp \
    $$PWD/model/propertycontainer.cpp \
    $$PWD/pluginmanager/widgetpluginmanager.cpp \
    $$PWD/pluginmanager/widgetpluginpath.cpp \
    $$PWD/exceptions/exception.cpp \
    $$PWD/exceptions/invalidpropertyexception.cpp \
    $$PWD/exceptions/invalidmodelnodeexception.cpp \
    $$PWD/exceptions/invalidreparentingexception.cpp \
    $$PWD/exceptions/invalidmetainfoexception.cpp \
    $$PWD/exceptions/invalidargumentexception.cpp \
    $$PWD/exceptions/notimplementedexception.cpp \
    $$PWD/exceptions/invalidmodelstateexception.cpp \
    $$PWD/exceptions/removebasestateexception.cpp \
    $$PWD/exceptions/invalididexception.cpp \
    $$PWD/model/propertynode.cpp \
    $$PWD/exceptions/invalidslideindexexception.cpp \
    $$PWD/model/import.cpp \
    $$PWD/exceptions/invalidqmlsourceexception.cpp \
    $$PWD/model/viewlogger.cpp \
    $$PWD/model/internalvariantproperty.cpp \
    $$PWD/model/internalnodelistproperty.cpp \
    $$PWD/model/variantproperty.cpp \
    $$PWD/model/nodelistproperty.cpp \
    $$PWD/model/abstractproperty.cpp \
    $$PWD/model/internalbindingproperty.cpp \
    $$PWD/model/bindingproperty.cpp \
    $$PWD/model/internalnodeproperty.cpp \
    $$PWD/model/internalnodeabstractproperty.cpp \
    $$PWD/model/nodeabstractproperty.cpp \
    $$PWD/model/nodeproperty.cpp \
    $$PWD/model/modeltotextmerger.cpp \
    $$PWD/model/texttomodelmerger.cpp \
    $$PWD/model/plaintexteditmodifier.cpp \
    $$PWD/model/componenttextmodifier.cpp \
    $$PWD/model/textmodifier.cpp \
    $$PWD/model/qmlmodelview.cpp \
    $$PWD/model/qmlitemnode.cpp \
    $$PWD/model/qmlstate.cpp \
    $$PWD/model/qmlchangeset.cpp \
    $$PWD/model/qmlmodelnodefacade.cpp \
    $$PWD/model/qmlobjectnode.cpp \
    $$PWD/model/qmlanchors.cpp \
    $$PWD/rewritertransaction.cpp \
    $$PWD/model/rewriteaction.cpp \
    $$PWD/model/modelnodepositionstorage.cpp \
    $$PWD/model/modelnodepositionrecalculator.cpp \
    $$PWD/model/rewriteactioncompressor.cpp \
    $$PWD/model/qmltextgenerator.cpp \
    $$PWD/model/modelmerger.cpp \
    $$PWD/exceptions/rewritingexception.cpp \
    $$PWD/model/viewmanager.cpp

HEADERS += $$PWD/include/qmldesignercorelib_global.h \
    $$PWD/include/abstractview.h \
    $$PWD/include/nodeinstanceview.h \
    $$PWD/include/rewriterview.h \
    $$PWD/include/metainfo.h \
    $$PWD/include/metainforeader.h \
    $$PWD/include/nodemetainfo.h \
    $$PWD/include/itemlibraryinfo.h \
    $$PWD/model/internalproperty.h \
    $$PWD/include/modelnode.h \
    $$PWD/include/model.h \
    $$PWD/include/nodeproperty.h \
    $$PWD/include/subcomponentmanager.h \
    $$PWD/include/propertycontainer.h \
    $$PWD/model/internalnode_p.h \
    $$PWD/model/model_p.h \
    $$PWD/model/painteventfilter_p.h \
    $$PWD/include/propertyparser.h \
    $$PWD/pluginmanager/widgetpluginmanager.h \
    $$PWD/pluginmanager/widgetpluginpath.h \
    $$PWD/include/exception.h \
    $$PWD/include/invalidreparentingexception.h \
    $$PWD/include/invalidmetainfoexception.h \
    $$PWD/include/invalidargumentexception.h \
    $$PWD/include/notimplementedexception.h \
    $$PWD/include/invalidpropertyexception.h \
    $$PWD/include/invalidmodelnodeexception.h \
    $$PWD/include/invalidmodelstateexception.h \
    $$PWD/include/removebasestateexception.h \
    $$PWD/include/invalididexception.h \
    $$PWD/include/propertynode.h \
    $$PWD/include/invalidslideindexexception.h \
    $$PWD/include/import.h \
    $$PWD/include/invalidqmlsourceexception.h \
    $$PWD/model/viewlogger.h \
    $$PWD/model/internalvariantproperty.h \
    $$PWD/model/internalnodelistproperty.h \
    $$PWD/include/variantproperty.h \
    $$PWD/include/nodelistproperty.h \
    $$PWD/include/abstractproperty.h \
    $$PWD/model/internalbindingproperty.h \
    $$PWD/include/bindingproperty.h \
    $$PWD/model/internalnodeproperty.h \
    $$PWD/model/internalnodeabstractproperty.h \
    $$PWD/include/nodeabstractproperty.h \
    $$PWD/include/plaintexteditmodifier.h \
    $$PWD/include/basetexteditmodifier.h \
    $$PWD/include/componenttextmodifier.h \
    $$PWD/include/textmodifier.h \
    $$PWD/model/modeltotextmerger.h \
    $$PWD/model/texttomodelmerger.h \
    $$PWD/include/qmlmodelview.h \
    $$PWD/include/qmlitemnode.h \
    $$PWD/include/qmlstate.h \
    $$PWD/include/qmlchangeset.h \
    $$PWD/include/qmlmodelnodefacade.h \
    $$PWD/include/forwardview.h \
    $$PWD/include/qmlobjectnode.h \
    $$PWD/include/qmlanchors.h \
    $$PWD/rewritertransaction.h \
    $$PWD/model/rewriteaction.h \
    $$PWD/include/modelnodepositionstorage.h \
    $$PWD/model/modelnodepositionrecalculator.h \
    $$PWD/model/rewriteactioncompressor.h \
    $$PWD/model/qmltextgenerator.h \
    $$PWD/include/modelmerger.h \
    $$PWD/include/mathutils.h \
    $$PWD/include/customnotifications.h \
    $$PWD/include/rewritingexception.h \
    $$PWD/include/viewmanager.h

contains(CONFIG, plugin) {
  # If core.pri has been included in the qmldesigner plugin
  SOURCES += $$PWD/model/basetexteditmodifier.cpp
  HEADERS += $$PWD/include/basetexteditmodifier.h
}
