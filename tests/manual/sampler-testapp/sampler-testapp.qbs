import qbs

QtcManualTest {
    name: "sampler-testapp"
    type: ["application"]

    Depends { name: "Qt.core" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.qml" }
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.quick3d"; required: false }

    builtByDefault: Qt.quick3d.present

    files: "main.cpp"
}
