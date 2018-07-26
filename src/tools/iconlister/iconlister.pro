SOURCES += \
    main.cpp \
    iconlister.cpp

HEADERS += \
    iconlister.h

QTC_LIB_DEPENDS += \
    utils \
    cplusplus

QTC_PLUGIN_DEPENDS += \
    autotest \
    coreplugin \
    debugger \
    projectexplorer \
    qmldesigner \
    diffeditor

include(../../../qtcreator.pri)

RESOURCES += \
    $$IDE_SOURCE_TREE/tests/manual/widgets/crumblepath/tst_crumblepath.qrc \
    $$IDE_SOURCE_TREE/src/plugins/autotest/autotest.qrc \
    $$IDE_SOURCE_TREE/src/plugins/debugger/debugger.qrc \
    $$IDE_SOURCE_TREE/src/plugins/help/help.qrc \
    $$IDE_SOURCE_TREE/src/plugins/diffeditor/diffeditor.qrc \
    $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/componentcore/componentcore.qrc \
    $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/formeditor/formeditor.qrc \
    $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/navigator/navigator.qrc \
    $$IDE_SOURCE_TREE/src/plugins/qmldesigner/components/resources/resources.qrc \
    $$IDE_SOURCE_TREE/src/plugins/welcome/welcome.qrc \
    $$IDE_SOURCE_TREE/src/plugins/baremetal/baremetal.qrc \
    $$IDE_SOURCE_TREE/src/plugins/ios/ios.qrc \
    $$IDE_SOURCE_TREE/src/plugins/android/android.qrc \
    $$IDE_SOURCE_TREE/src/plugins/qnx/qnx.qrc \
    $$IDE_SOURCE_TREE/src/plugins/winrt/winrt.qrc \
    $$IDE_SOURCE_TREE/src/libs/tracing/qml/tracing.qrc \

DEFINES += \
    IDE_SOURCE_TREE='\\"$$IDE_SOURCE_TREE\\"'
