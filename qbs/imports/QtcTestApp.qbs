import qbs
import qbs.FileInfo

CppApplication {
    type: ["application"]

    Depends { name: "copyable_resource" }

    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
}
