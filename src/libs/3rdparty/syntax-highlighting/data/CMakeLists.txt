# generate PHP definitions
macro(generate_php_syntax_definition targetFile srcFile)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/syntax)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generators/generate-php.pl
        INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/syntax/${srcFile}
        OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/syntax/${targetFile})
endmacro()

generate_php_syntax_definition(javascript-php.xml javascript.xml)
generate_php_syntax_definition(css-php.xml css.xml)
generate_php_syntax_definition(html-php.xml html.xml)

# find all definitions
file(GLOB src_defs "${CMAKE_CURRENT_SOURCE_DIR}/syntax/*.xml")
set(defs
    ${src_defs}
    ${CMAKE_CURRENT_BINARY_DIR}/syntax/html-php.xml
    ${CMAKE_CURRENT_BINARY_DIR}/syntax/css-php.xml
    ${CMAKE_CURRENT_BINARY_DIR}/syntax/javascript-php.xml
)

# theme data resource
qt5_add_resources(themes_QRC ${CMAKE_CURRENT_SOURCE_DIR}/themes/theme-data.qrc)

# do we want syntax files bundled in the library?
if (QRC_SYNTAX)
    # generate the resource file
    set(qrc_file ${CMAKE_CURRENT_BINARY_DIR}/syntax-data.qrc)
    set(qrc_body "")
    foreach(def ${defs})
        get_filename_component(def_name ${def} NAME)
        string(APPEND qrc_body "<file alias=\"${def_name}\">${def}</file>\n")
    endforeach()
    set(SYNTAX_DATA_QRC_FILES_STRING ${qrc_body})
    configure_file(syntax-data.qrc.in ${qrc_file} @ONLY)

    # generate the index file
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax"
    COMMAND katehighlightingindexer "${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax" "${CMAKE_CURRENT_SOURCE_DIR}/schema/language.xsd" "${CMAKE_CURRENT_BINARY_DIR}/syntax-data.qrc"
    DEPENDS ${defs} ${CMAKE_CURRENT_SOURCE_DIR}/schema/language.xsd ${CMAKE_CURRENT_BINARY_DIR}/syntax-data.qrc
    )

    # generate the qrc file manually, to make dependencies on generated files work...
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp"
        COMMAND ${Qt5Core_RCC_EXECUTABLE} --name syntax_data -o "${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp" "${CMAKE_CURRENT_BINARY_DIR}/syntax-data.qrc"
        DEPENDS ${defs} ${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax
    )
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp" PROPERTIES SKIP_AUTOMOC ON)

    # object library to make cross-folder dependencies work, themes + syntax files
    add_library(SyntaxHighlightingData OBJECT ${themes_QRC} ${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp)
else()
    # install the syntax files as normal files into the prefix
    install (FILES ${defs} DESTINATION share/org.kde.syntax-highlighting/syntax)

    # object library to make cross-folder dependencies work, only themes
    add_library(SyntaxHighlightingData OBJECT ${themes_QRC})
endif()

# set PIC to allow use in static and shared libs
set_property(TARGET SyntaxHighlightingData PROPERTY POSITION_INDEPENDENT_CODE 1)
