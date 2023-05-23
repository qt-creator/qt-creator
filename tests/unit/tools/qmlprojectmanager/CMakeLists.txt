if(NOT TARGET QmlProjectManagerLib)
    return()
endif()

project(QmlProjectManagerConverterDataCreator)

add_compile_definitions(QT_CREATOR)

add_executable(${PROJECT_NAME}
    main.cpp
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
       OUTPUT_NAME "dataSetGenerator"
)

target_link_libraries(${PROJECT_NAME}
    QmlJS Utils ProjectExplorer QmlProjectManagerLib
)
