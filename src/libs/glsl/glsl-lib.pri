HEADERS += $$PWD/glsl.h $$PWD/glsllexer.h $$PWD/glslparser.h glslparsertable_p.h $$PWD/glslast.h
SOURCES += $$PWD/glslkeywords.cpp $$PWD/glslparser.cpp $$PWD/glslparsertable.cpp \
    $$PWD/glsllexer.cpp $$PWD/glslast.cpp $$PWD/glsldump.cpp $$PWD/glsldelete.cpp

OTHER_FILES = $$PWD/specs/glsl.g.in \
    $$PWD/specs/grammar.txt
