include(../cplusplus-shared/tool.pri)

DEFINES += PATH_CPP_FRONTEND=\\\"$$PWD/../../libs/3rdparty/cplusplus\\\"
DEFINES += PATH_DUMPERS_FILE=\\\"$$PWD/../cplusplus-ast2png/dumpers.inc\\\"

SOURCES += cplusplus-update-frontend.cpp
