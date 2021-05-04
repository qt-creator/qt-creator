INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/designercore/include
INCLUDEPATH += $$PWD/designercore/imagecache
INCLUDEPATH += $$PWD/designercore
INCLUDEPATH += $$PWD/../../../share/qtcreator/qml/qmlpuppet/interfaces
INCLUDEPATH += $$PWD/../../../share/qtcreator/qml/qmlpuppet/types

DEFINES += QMLDESIGNER_TEST DESIGNER_STATIC_CORE_LIBRARY

include($$PWD/designercore/exceptions/exceptions.pri)

SOURCES += \
    $$PWD/designercore/model/model.cpp \
    $$PWD/designercore/model/modelnode.cpp \
    $$PWD/designercore/model/import.cpp \
    $$PWD/designercore/model/abstractproperty.cpp \
    $$PWD/designercore/model/abstractview.cpp \
    $$PWD/designercore/model/internalproperty.cpp \
    $$PWD/designercore/model/internalbindingproperty.cpp \
    $$PWD/designercore/model/internalnodeabstractproperty.cpp \
    $$PWD/designercore/model/internalnodelistproperty.cpp \
    $$PWD/designercore/model/internalnodeproperty.cpp \
    $$PWD/designercore/model/internalsignalhandlerproperty.cpp \
    $$PWD/designercore/model/internalnode.cpp \
    $$PWD/designercore/model/internalvariantproperty.cpp \
    $$PWD/designercore/model/bindingproperty.cpp \
    $$PWD/designercore/model/nodeabstractproperty.cpp \
    $$PWD/designercore/model/nodelistproperty.cpp \
    $$PWD/designercore/model/nodeproperty.cpp \
    $$PWD/designercore/model/signalhandlerproperty.cpp \
    $$PWD/designercore/model/variantproperty.cpp\
    $$PWD/designercore/model/annotation.cpp \
    $$PWD/designercore/rewritertransaction.cpp \
    $$PWD/components/listmodeleditor/listmodeleditormodel.cpp \
    $$PWD/designercore/imagecache/asynchronousimagecache.cpp \
    $$PWD/designercore/imagecache/synchronousimagecache.cpp \
    $$PWD/designercore/imagecache/imagecachegenerator.cpp \
    $$PWD/designercore/projectstorage/projectstoragesqlitefunctionregistry.cpp

HEADERS += \
    $$PWD/designercore/imagecache/imagecachecollectorinterface.h \
    $$PWD/designercore/imagecache/imagecachestorage.h \
    $$PWD/designercore/imagecache/imagecachegenerator.h \
    $$PWD/designercore/imagecache/imagecachestorageinterface.h \
    $$PWD/designercore/imagecache/imagecachegeneratorinterface.h \
    $$PWD/designercore/imagecache/timestampproviderinterface.h \
    $$PWD/designercore/include/asynchronousimagecache.h \
    $$PWD/designercore/include/synchronousimagecache.h \
    $$PWD/designercore/include/imagecacheauxiliarydata.h \
    $$PWD/designercore/include/asynchronousimagecacheinterface.h \
    $$PWD/designercore/include/modelnode.h \
    $$PWD/designercore/include/model.h \
    $$PWD/../../../share/qtcreator/qml/qmlpuppet/interfaces/commondefines.h \
    $$PWD/designercore/include/import.h \
    $$PWD/designercore/include/abstractproperty.h \
    $$PWD/designercore/include/abstractview.h \
    $$PWD/designercore/projectstorage/storagecache.h \
    $$PWD/designercore/projectstorage/storagecacheentry.h \
    $$PWD/designercore/projectstorage/storagecachefwd.h \
    $$PWD/designercore/projectstorage/sourcepath.h \
    $$PWD/designercore/projectstorage/sourcepathview.h \
    $$PWD/designercore/projectstorage/sourcepathcache.h \
    $$PWD/designercore/model/model_p.h \
    $$PWD/designercore/include/qmldesignercorelib_global.h \
    $$PWD/designercore/model/internalbindingproperty.h \
    $$PWD/designercore/model/internalnode_p.h \
    $$PWD/designercore/model/internalnodeabstractproperty.h \
    $$PWD/designercore/model/internalnodelistproperty.h \
    $$PWD/designercore/model/internalnodeproperty.h \
    $$PWD/designercore/model/internalproperty.h \
    $$PWD/designercore/model/internalsignalhandlerproperty.h \
    $$PWD/designercore/model/internalvariantproperty.h \
    $$PWD/designercore/include/bindingproperty.h \
    $$PWD/designercore/include/nodeabstractproperty.h \
    $$PWD/designercore/include/nodelistproperty.h \
    $$PWD/designercore/include/nodeproperty.h \
    $$PWD/designercore/include/signalhandlerproperty.h \
    $$PWD/designercore/include/variantproperty.h \
    $$PWD/designercore/rewritertransaction.h \
    $$PWD/components/listmodeleditor/listmodeleditormodel.h \
    $$PWD/designercore/projectstorage/projectstorage.h \
    $$PWD/designercore/include/projectstorageids.h \
    $$PWD/designercore/projectstorage/projectstoragetypes.h \
    $$PWD/designercore/projectstorage/projectstoragesqlitefunctionregistry.h \
    $$PWD/designercore/projectstorage/sourcepathcache.h
