Project {
    name: "Solutions"

    references: [
        "spinner/spinner.qbs",
        "tasking/tasking.qbs",
    ].concat(project.additionalLibs)
}
