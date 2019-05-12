add_qtc_library(GLSL
  DEPENDS Qt5::Core
  SOURCES
    glsl.h
    glslast.cpp glslast.h
    glslastdump.cpp glslastdump.h
    glslastvisitor.cpp glslastvisitor.h
    glslengine.cpp glslengine.h
    glslkeywords.cpp
    glsllexer.cpp glsllexer.h
    glslmemorypool.cpp glslmemorypool.h
    glslparser.cpp glslparser.h
    glslparsertable.cpp glslparsertable_p.h
    glslsemantic.cpp glslsemantic.h
    glslsymbol.cpp glslsymbol.h
    glslsymbols.cpp glslsymbols.h
    glsltype.cpp glsltype.h
    glsltypes.cpp glsltypes.h
)
