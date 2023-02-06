import QmlProject 1.3

Project {
    MCU.Module {
        uri: "CustomModule"
        generateQmltypes: true
    }

    QmlFiles {
        files: ["%{QmlComponent}"]
    }

    ModuleFiles {
        MCU.qulModules: ["Qul::ControlsTemplates"]
    }
}
