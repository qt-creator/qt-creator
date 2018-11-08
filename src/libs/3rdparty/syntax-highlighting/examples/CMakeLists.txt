if(Qt5Widgets_FOUND)
    add_executable(codeeditor codeeditor.cpp main.cpp)
    target_link_libraries(codeeditor Qt5::Widgets KF5SyntaxHighlighting)
endif()
