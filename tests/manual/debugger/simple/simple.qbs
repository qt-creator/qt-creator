import qbs
import qbs.FileInfo

Project {
    name: "Manual debugger simple tests"

    condition: project.withAutotests

    references: [
        "simple_test_app.qbs",
        "simple_test_plugin.qbs"
    ]
}
