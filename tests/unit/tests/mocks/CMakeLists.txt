add_qtc_library(TestMocks OBJECT
  EXCLUDE_FROM_INSTALL
  SKIP_AUTOMOC ON
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  DEPENDS
    Qt::Core Qt::Widgets Googletest Sqlite TestDesignerCore
  SOURCES
    abstractviewmock.h
    externaldependenciesmock.h
    filesystemmock.h
    imagecachecollectormock.h
    mockimagecachegenerator.h
    mockimagecachestorage.h
    mocklistmodeleditorview.h
    mockmutex.h
    mockqfilesystemwatcher.h
    mocksqlitestatement.h
    mocksqlitetransactionbackend.h
    mocksyntaxhighligher.h
    mocktimer.cpp
    mocktimer.h
    mocktimestampprovider.h
    modelresourcemanagementmock.h
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
