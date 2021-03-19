if (WITH_TESTS)
  set(QT_TEST_COMPONENT Test)
  set(IMPLICIT_DEPENDS Qt5::Test)
endif()

find_package(QtCreator COMPONENTS Core TextEditor ProjectExplorer VcsBase REQUIRED)
find_package(Qt5 COMPONENTS Widgets ${QT_TEST_COMPONENT} REQUIRED)

add_qtc_plugin(Fossil
  PLUGIN_DEPENDS
    QtCreator::Core QtCreator::TextEditor QtCreator::ProjectExplorer QtCreator::VcsBase
  DEPENDS Qt5::Widgets QtCreator::ExtensionSystem QtCreator::Utils
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    branchinfo.cpp branchinfo.h
    commiteditor.cpp commiteditor.h
    configuredialog.cpp configuredialog.h configuredialog.ui
    constants.h
    fossil.qrc
    fossilclient.cpp fossilclient.h
    fossilcommitpanel.ui
    fossilcommitwidget.cpp fossilcommitwidget.h
    fossileditor.cpp fossileditor.h
    fossilplugin.cpp fossilplugin.h
    fossilsettings.cpp fossilsettings.h
    pullorpushdialog.cpp pullorpushdialog.h pullorpushdialog.ui
    revertdialog.ui
    revisioninfo.cpp revisioninfo.h
    wizard/fossiljsextension.cpp wizard/fossiljsextension.h
)
