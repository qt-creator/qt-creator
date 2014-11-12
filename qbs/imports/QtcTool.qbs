import qbs 1.0

QtcProduct {
    type: "application" // no Mac app bundle
    installDir:  project.ide_libexec_path

    cpp.rpaths: qbs.targetOS.contains("osx")
            ? ["@executable_path/../" + project.ide_library_path]
            : ["$ORIGIN/../" + project.ide_library_path]
}
