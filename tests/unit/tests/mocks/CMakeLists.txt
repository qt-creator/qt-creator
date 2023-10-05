add_qtc_library(TestMocks OBJECT
  EXCLUDE_FROM_INSTALL
  PROPERTIES SKIP_AUTOGEN ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  INCLUDES ../../../../src/plugins/qmldesigner/components/componentcore
  DEPENDS
    Qt::Core Qt::Widgets Googletest Sqlite TestDesignerCore
  SOURCES
    abstractviewmock.h
    externaldependenciesmock.h
    filesystemmock.h
    imagecachecollectormock.h
    mockimagecachegenerator.h
    mockimagecachestorage.h
    mockmutex.h
    mockqfilesystemwatcher.h
    mocksqlitestatement.h
    mocksqlitetransactionbackend.h
    mocksyntaxhighligher.h
    mocktimer.cpp
    mocktimer.h
    mocktimestampprovider.h
    modelresourcemanagementmock.h
    propertycomponentgeneratormock.h
    projectstoragemock.cpp
    projectstoragemock.h
    projectstoragepathwatchermock.h
    projectstoragepathwatchernotifiermock.h
    qmldocumentparsermock.h
    qmltypesparsermock.h
    sourcepathcachemock.h sourcepathcachemock.cpp
    sqlitedatabasemock.h
    sqlitereadstatementmock.cpp
    sqlitereadstatementmock.h
    sqlitereadwritestatementmock.cpp
    sqlitereadwritestatementmock.h
    sqlitestatementmock.h
    sqlitetransactionbackendmock.h
    sqlitewritestatementmock.cpp
    sqlitewritestatementmock.h
)
