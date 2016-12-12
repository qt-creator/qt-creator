import qbs 1.0

QtcLibrary {
    name: "GLSL"

    cpp.defines: base.concat([
        "GLSL_LIBRARY"
    ])

    Depends { name: "Qt.gui" }

    files: [
        "glsl.g",
        "glsl.h",
        "glslast.cpp",
        "glslast.h",
        "glslastdump.cpp",
        "glslastdump.h",
        "glslastvisitor.cpp",
        "glslastvisitor.h",
        "glslengine.cpp",
        "glslengine.h",
        "glslkeywords.cpp",
        "glsllexer.cpp",
        "glsllexer.h",
        "glslmemorypool.cpp",
        "glslmemorypool.h",
        "glslparser.cpp",
        "glslparser.h",
        "glslparsertable.cpp",
        "glslparsertable_p.h",
        "glslsemantic.cpp",
        "glslsemantic.h",
        "glslsymbol.cpp",
        "glslsymbol.h",
        "glslsymbols.cpp",
        "glslsymbols.h",
        "glsltype.cpp",
        "glsltype.h",
        "glsltypes.cpp",
        "glsltypes.h",
    ]
}
