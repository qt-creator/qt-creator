set(QML_SOURCES
    Main.qml
)

qt_add_executable(appQuickApp
    main.cpp
)

qt_add_qml_module(appQuickApp
    URI QuickApp
    VERSION 1.0
    QML_FILES
        ${QML_SOURCES}
)
