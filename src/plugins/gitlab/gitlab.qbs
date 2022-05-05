import qbs

QtcPlugin {
    name: "GitLab"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Git" }
    Depends { name: "VcsBase" }
    Depends { name: "Utils" }

    files: [
        "gitlaboptionspage.cpp",
        "gitlaboptionspage.h",
        "gitlabparameters.cpp",
        "gitlabparameters.h",
        "gitlabplugin.cpp",
        "gitlabplugin.h",
        "gitlabprojectsettings.cpp",
        "gitlabprojectsettings.h",
        "queryrunner.cpp",
        "queryrunner.h",
        "resultparser.cpp",
        "resultparser.h",
    ]
}
