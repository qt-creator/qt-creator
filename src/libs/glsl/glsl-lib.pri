HEADERS += $$PWD/glsl.h $$PWD/glsllexer.h $$PWD/glslparser.h $$PWD/glslparsertable_p.h $$PWD/glslast.h \
    $$PWD/glslastvisitor.h $$PWD/glslengine.h $$PWD/glslmemorypool.h $$PWD/glslastdump.h \
    $$PWD/glslsemantic.h $$PWD/glsltype.h $$PWD/glsltypes.h $$PWD/glslsymbol.h $$PWD/glslsymbols.h
SOURCES += $$PWD/glslkeywords.cpp $$PWD/glslparser.cpp $$PWD/glslparsertable.cpp \
    $$PWD/glsllexer.cpp $$PWD/glslast.cpp \
    $$PWD/glslastvisitor.cpp $$PWD/glslengine.cpp $$PWD/glslmemorypool.cpp $$PWD/glslastdump.cpp \
    $$PWD/glslsemantic.cpp $$PWD/glsltype.cpp $$PWD/glsltypes.cpp $$PWD/glslsymbol.cpp $$PWD/glslsymbols.cpp

DISTFILES = $$PWD/glsl.g
