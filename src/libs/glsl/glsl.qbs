import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "GLSL"

    cpp.includePaths: [
        ".",
        ".."
    ]
    cpp.defines: base.concat([
        "QT_CREATOR",
        "GLSL_BUILD_LIB"
    ])

    Depends { name: "cpp" }
    Depends { name: "Qt.gui" }

    files: [
        "glsl.h",
        "glsllexer.h",
        "glslparser.h",
        "glslparsertable_p.h",
        "glslast.h",
        "glslastvisitor.h",
        "glslengine.h",
        "glslmemorypool.h",
        "glslastdump.h",
        "glslsemantic.h",
        "glsltype.h",
        "glsltypes.h",
        "glslsymbol.h",
        "glslsymbols.h",
        "glslkeywords.cpp",
        "glslparser.cpp",
        "glslparsertable.cpp",
        "glsllexer.cpp",
        "glslast.cpp",
        "glslastvisitor.cpp",
        "glslengine.cpp",
        "glslmemorypool.cpp",
        "glslastdump.cpp",
        "glslsemantic.cpp",
        "glsltype.cpp",
        "glsltypes.cpp",
        "glslsymbol.cpp",
        "glslsymbols.cpp",
        "glsl.g"
    ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: ["."]
    }
}

