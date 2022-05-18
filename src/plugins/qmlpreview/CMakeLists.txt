add_qtc_plugin(QmlPreview
  CONDITION TARGET QmlProjectManager
  PUBLIC_DEPENDS QmlDebug
  DEPENDS QmlJS
  PLUGIN_DEPENDS
    Core ProjectExplorer QmlJSTools QtSupport
    ResourceEditor QmlProjectManager
  SOURCES
    qmlpreviewclient.cpp qmlpreviewclient.h
    qmlpreviewconnectionmanager.cpp qmlpreviewconnectionmanager.h
    qmlpreviewfileontargetfinder.cpp qmlpreviewfileontargetfinder.h
    qmlpreviewplugin.cpp qmlpreviewplugin.h
    qmlpreviewruncontrol.cpp qmlpreviewruncontrol.h
    qmldebugtranslationclient.cpp qmldebugtranslationclient.h
    qmlpreview_global.h
)

extend_qtc_plugin(QmlPreview
  CONDITION WITH_TESTS
  SOURCES
    tests/qmlpreviewclient_test.cpp tests/qmlpreviewclient_test.h
    tests/qmlpreviewplugin_test.cpp tests/qmlpreviewplugin_test.h
)

if(TARGET Qt${QT_VERSION_MAJOR}::QmlPrivate)
  # check if Qt version have_qml_debug_translation_protocol
  # will be introduced in Qt 6.2, but there are users
  # who needs it in older but special built Qt versions aswell
  string(REGEX MATCH "^[0-9]*" QT_VERSION_MAJOR ${Qt5_VERSION})
  get_target_property(qmldebugprivate_include_directories
                      Qt${QT_VERSION_MAJOR}::QmlPrivate
                      INTERFACE_INCLUDE_DIRECTORIES
  )
  find_file(have_qml_debug_translation_protocol
    NAMES private/qqmldebugtranslationprotocol_p.h
    PATHS ${qmldebugprivate_include_directories}
  )
  extend_qtc_plugin(QmlPreview
    CONDITION have_qml_debug_translation_protocol
    PUBLIC_DEPENDS Qt5::QmlPrivate
    PUBLIC_DEFINES "FOUND_QML_DEBUG_TRANSLATION_PROTOCOL"
  )
endif()
