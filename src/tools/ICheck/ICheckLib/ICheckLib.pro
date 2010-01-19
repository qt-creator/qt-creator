REL_PATH_TO_SRC = ../../../

TEMPLATE = lib

CONFIG(debug, debug|release){
    TARGET = ICheckLibd
    DESTDIR =  ./debug
    DLLDESTDIR = ../ICheckApp/debug
    MOC_DIR += ./debug/GeneratedFiles
    OBJECTS_DIR += ./debug
    INCLUDEPATH += ./ \
        ./debug/GeneratedFiles \
        $$REL_PATH_TO_SRC/plugins \
        $$REL_PATH_TO_SRC/libs \
        $$REL_PATH_TO_SRC/shared/cplusplus \
        $$REL_PATH_TO_SRC/libs/cplusplus \
        $(QTDIR)
}
else {
    TARGET = ICheckLib
    DESTDIR = ./release
    DLLDESTDIR = ../ICheckApp/release
    MOC_DIR += ./release/GeneratedFiles
    OBJECTS_DIR += ./release
    INCLUDEPATH += ./ \
        ./release/GeneratedFiles \
        $$REL_PATH_TO_SRC/plugins \
        $$REL_PATH_TO_SRC/libs \
        $$REL_PATH_TO_SRC/shared/cplusplus \
        $$REL_PATH_TO_SRC/libs/cplusplus \
        $(QTDIR)
}

DEFINES += ICHECKLIB_LIBRARY ICHECK_BUILD
include(ICheckLib.pri)
