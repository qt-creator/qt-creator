qt_add_executable(appTestQuick
    main.cpp
)

qt_add_qml_module(appTestQuick
    URI TestQuick
    QML_FILES Main2.qml Other.qml
)
