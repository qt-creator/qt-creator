INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/clangrefactoringbackend_global.h \
    $$PWD/sourcerangefilter.h

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
    $$PWD/locationsourcefilecallbacks.cpp \
    $$PWD/clangquerygatherer.cpp

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
    $$PWD/locationsourcefilecallbacks.h \
    $$PWD/clangquerygatherer.h
}

SOURCES += \
    $$PWD/sourcerangefilter.cpp
