QtcLibrary {
    name: "TextEditorSupport"
    type: "staticlibrary"

    Depends { name: "Utils" }

    cpp.defines: base.concat("TEXTEDITORSUPPORT_STATIC_LIBRARY")

    files: [
        "tabsettingsdata.cpp",
        "texteditorsupport_global.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.sourceDirectory
        cpp.defines: ["TEXTEDITORSUPPORT_STATIC_LIBRARY"]
    }
}
