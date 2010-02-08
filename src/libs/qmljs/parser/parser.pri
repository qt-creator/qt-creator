INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qmljsast_p.h \
    $$PWD/qmljsastfwd_p.h \
    $$PWD/qmljsastvisitor_p.h \
    $$PWD/qmljsengine_p.h \
    $$PWD/qmljsgrammar_p.h \
    $$PWD/qmljslexer_p.h \
    $$PWD/qmljsmemorypool_p.h \
    $$PWD/qmljsnodepool_p.h \
    $$PWD/qmljsparser_p.h \
    $$PWD/qmljsglobal_p.h

SOURCES += \
    $$PWD/qmljsast.cpp \
    $$PWD/qmljsastvisitor.cpp \
    $$PWD/qmljsengine_p.cpp \
    $$PWD/qmljsgrammar.cpp \
    $$PWD/qmljslexer.cpp \
    $$PWD/qmljsparser.cpp
