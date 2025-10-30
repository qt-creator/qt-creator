Project {
    name: "Solutions"

    references: [
        "spinner/spinner.qbs",
        "terminal/terminal.qbs",
    ].concat(project.additionalLibs)
}
