StaticLibrary {
    Group {
        condition: qbs.targetOS.contains("darwin")
        Depends { name: "bundle" }
        product.bundle.isBundle: false
    }
}
