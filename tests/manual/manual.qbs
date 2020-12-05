import qbs

Project {
    name: "QtcManualtests"

    condition: project.withAutotests

    references: [
        "widgets/widgets.qbs",
    ]
}
