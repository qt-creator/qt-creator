Product {
    name: "sol2"

    Group {
        prefix: "include/"
        files: [
            "sol/config.hpp",
            "sol/forward.hpp",
            "sol/sol.hpp",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.ide_source_tree + "/src/libs/3rdparty/sol2/include"
    }
}
