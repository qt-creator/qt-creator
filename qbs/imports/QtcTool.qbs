import qbs
import qbs.FileInfo

QtcProduct {
    type: "application" // no Mac app bundle
    installDir:  project.ide_libexec_path

    cpp.rpaths: {
        var relativePathToLibs
                = FileInfo.relativePath(project.ide_libexec_path, project.ide_library_path);
        var relativePathToPlugins
                = FileInfo.relativePath(project.ide_libexec_path, project.ide_plugin_path);
        var prefix = qbs.targetOS.contains("osx") ? "@executable_path" : "$ORIGIN";
        return [
            FileInfo.joinPaths(prefix, relativePathToLibs),
            FileInfo.joinPaths(prefix, relativePathToPlugins)
        ];
    }
}
