import qbs

QtcAutotest {
    Depends { name: "Tracing"; required: false }
    Depends { name: "Qt"; submodules: [ "widgets" ] }

    condition: Tracing.present
}
