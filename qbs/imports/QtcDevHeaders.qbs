import qbs
import qbs.FileInfo

Product {
    property string productName: project.name
    name: productName + " dev headers"
    condition: qtc.make_dev_package
    Depends { name: "qtc" }
    Group {
        files: ["**/*.h"]
        qbs.install: true
        qbs.installDir: qtc.ide_include_path + '/' + FileInfo.fileName(product.sourceDirectory)
        qbs.installSourceBase: product.sourceDirectory
    }
}
