QtcLibrary {
    name: "TextEditorSupport"
    type: "staticlibrary"

    Depends { name: "Utils" }

    cpp.defines: base.concat("TEXTEDITORSUPPORT_STATIC_LIBRARY")

    files: [
        "tabsettings.cpp",
        "tabsettings.h",
        "texteditorsupport_global.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.sourceDirectory
        cpp.defines: ["TEXTEDITORSUPPORT_STATIC_LIBRARY"]
    }
}
