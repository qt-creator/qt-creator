HEADERS += $$PWD/glsl.h $$PWD/glsllexer.h $$PWD/glslparser.h $$PWD/glslparsertable_p.h $$PWD/glslast.h \
    $$PWD/glslastvisitor.h $$PWD/glslengine.h $$PWD/glslmemorypool.h
SOURCES += $$PWD/glslkeywords.cpp $$PWD/glslparser.cpp $$PWD/glslparsertable.cpp \
    $$PWD/glsllexer.cpp $$PWD/glslast.cpp \
    $$PWD/glslastvisitor.cpp $$PWD/glslengine.cpp $$PWD/glslmemorypool.cpp

OTHER_FILES = $$PWD/glsl.g \
    $$PWD/specs/grammar.txt
