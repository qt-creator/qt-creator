import qbs
import qbs.FileInfo

Product {
    property string productName: project.name
    property string baseDir: sourceDirectory
    name: productName + " dev headers"
    condition: qtc.make_dev_package
    Depends { name: "qtc" }
    Group {
        prefix: baseDir + '/'
        files: ["**/*.h"]
        qbs.install: true
        qbs.installDir: qtc.ide_include_path + '/' + FileInfo.fileName(product.sourceDirectory)
        qbs.installSourceBase: baseDir
    }
}
