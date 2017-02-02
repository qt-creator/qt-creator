INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/clangrefactoringbackend_global.h \

!isEmpty(LIBTOOLING_LIBS) {
SOURCES += \
    $$PWD/refactoringcompilationdatabase.cpp \
    $$PWD/symbolfinder.cpp \
    $$PWD/symbollocationfinderaction.cpp \
    $$PWD/refactoringserver.cpp \
    $$PWD/macropreprocessorcallbacks.cpp \
    $$PWD/findusrforcursoraction.cpp \
    $$PWD/clangquery.cpp \
    $$PWD/clangtool.cpp \
    $$PWD/sourcerangeextractor.cpp \
    $$PWD/locationsourcefilecallbacks.cpp

HEADERS += \
    $$PWD/refactoringcompilationdatabase.h \
    $$PWD/symbolfinder.h \
    $$PWD/symbollocationfinderaction.h \
    $$PWD/refactoringserver.h \
    $$PWD/macropreprocessorcallbacks.h \
    $$PWD/sourcelocationsutils.h \
    $$PWD/findcursorusr.h \
    $$PWD/findusrforcursoraction.h \
    $$PWD/findlocationsofusrs.h \
    $$PWD/clangquery.h \
    $$PWD/clangtool.h \
    $$PWD/sourcerangeextractor.h \
    $$PWD/locationsourcefilecallbacks.h
}
