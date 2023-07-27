Project {
    name: "Solutions"

    references: [
        "spinner/spinner.qbs",
        "tasking/tasking.qbs",
        "terminal/terminal.qbs",
    ].concat(project.additionalLibs)
}
