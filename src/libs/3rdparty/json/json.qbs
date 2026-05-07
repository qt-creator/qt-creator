Product {
    name: "Json"
    Depends { name: "cpp" }
    files: "json.hpp"
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.sourceDirectory + "/.."
    }
}
