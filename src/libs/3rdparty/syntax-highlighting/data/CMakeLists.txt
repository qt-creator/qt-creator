# create a directory for generated definitions
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/generated/syntax)

macro(generate_syntax_definition generator targetFile srcFile)
    add_custom_target(
        ${targetFile} ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated/syntax/${targetFile}
    )
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/syntax/${targetFile}
        COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generators/${generator}
                ${CMAKE_CURRENT_SOURCE_DIR}/syntax/${srcFile}
                ${CMAKE_CURRENT_BINARY_DIR}/generated/syntax/${targetFile}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generators/${generator}
                ${CMAKE_CURRENT_SOURCE_DIR}/syntax/${srcFile}
                ${ARGN}
        VERBATIM
    )
    set(gen_defs ${gen_defs} ${CMAKE_CURRENT_BINARY_DIR}/generated/syntax/${targetFile})
endmacro()

macro(generate_syntax_definition_with_html syntax mergedSyntax)
    generate_syntax_definition(generate-html.pl ${mergedSyntax}-${syntax}.xml ${mergedSyntax}.xml ${CMAKE_CURRENT_SOURCE_DIR}/syntax/${syntax}.xml)
endmacro()

# generate PHP definitions
generate_syntax_definition_with_html(php html)
generate_syntax_definition_with_html(php css)
generate_syntax_definition_with_html(php javascript)
generate_syntax_definition_with_html(php javascript-react)
generate_syntax_definition_with_html(php typescript)
generate_syntax_definition_with_html(php mustache)

# generate Twig definitions
generate_syntax_definition_with_html(twig html)
generate_syntax_definition_with_html(twig css)
generate_syntax_definition_with_html(twig javascript)
generate_syntax_definition_with_html(twig javascript-react)
generate_syntax_definition_with_html(twig typescript)

# generate DoxygenLua definition
generate_syntax_definition(generate-doxygenlua.pl doxygenlua.xml doxygen.xml)

# generate XSLT definition
generate_syntax_definition(generate-xslt.pl xslt.xml xml.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/generators/xslt.base.xml
)

# generate JSONC definition
generate_syntax_definition(generate-jsonc.pl jsonc.xml json.xml)

# generate Jinja definitions
if(Python_FOUND)
    set(jinja_langs html css xml json yaml toml sql python cpp qml java groovy rust javascript typescript ruby cmake qmake ini bash zsh dockerfile terraform systemd-unit)
    set(jinja_indir ${CMAKE_CURRENT_SOURCE_DIR}/syntax)
    set(jinja_outdir ${CMAKE_CURRENT_BINARY_DIR}/generated/syntax)
    set(jinja_prefix jinja-)
    list(TRANSFORM jinja_langs REPLACE "(.+)" "${jinja_indir}/\\1.xml" OUTPUT_VARIABLE in_xmls)

    execute_process(
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generators/generate_jinja.py --dry-run
                --jinja-xml ${CMAKE_CURRENT_SOURCE_DIR}/syntax/jinja.xml
                --prefix ${jinja_prefix}
                --output-dir ${jinja_outdir}
                ${in_xmls}
        OUTPUT_VARIABLE out_xmls
        RESULT_VARIABLE dry_run
    )

    if(${dry_run} EQUAL 0)
        string(REPLACE "\r" "" out_xmls ${out_xmls})
        string(REPLACE "\n" ";" out_xmls ${out_xmls})
        file(TO_CMAKE_PATH "${out_xmls}" out_xmls)
        list(TRANSFORM out_xmls REPLACE ${jinja_outdir} ${jinja_indir} OUTPUT_VARIABLE in_xmls)
        list(TRANSFORM in_xmls REPLACE ${jinja_prefix} "")

        add_custom_target(
            generate-jinja ALL
            DEPENDS ${out_xmls}
        )
        add_custom_command(
            OUTPUT ${out_xmls}
            COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generators/generate_jinja.py
                    --jinja-xml ${CMAKE_CURRENT_SOURCE_DIR}/syntax/jinja.xml
                    --prefix ${jinja_prefix}
                    --output-dir ${jinja_outdir}
                    ${in_xmls}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generators/generate_jinja.py
                    ${CMAKE_CURRENT_SOURCE_DIR}/syntax/jinja.xml
                    ${in_xmls}
            VERBATIM
        )
        set(gen_defs ${gen_defs} ${out_xmls})
    else()
        message(NOTICE "There was an error with the Jinja generator tool, left out of build.")
    endif()
endif()

# find all definitions
file(GLOB src_defs "${CMAKE_CURRENT_SOURCE_DIR}/syntax/*.xml")
set(defs ${src_defs} ${gen_defs})

# object library to make cross-folder dependencies work
add_library(SyntaxHighlightingData OBJECT)

# theme data resource
target_sources(SyntaxHighlightingData PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/themes/theme-data.qrc)

# generate the resource file
set(compressed_xml_dir ${CMAKE_CURRENT_BINARY_DIR}/generated/compressed-syntax)
set(qrc_xml_file ${CMAKE_CURRENT_BINARY_DIR}/syntax-data-xml.qrc)
set(qrc_file ${CMAKE_CURRENT_BINARY_DIR}/syntax-data.qrc)
set(qrc_xml_body "")
set(qrc_body "")
foreach(def ${defs})
    get_filename_component(def_name ${def} NAME)
    string(APPEND qrc_body "<file alias=\"${def_name}\">${compressed_xml_dir}/${def_name}</file>\n")
    string(APPEND qrc_xml_body "<file>${def}</file>\n")
    set(compressedDefs ${compressedDefs} ${compressed_xml_dir}/${def_name})
endforeach()
write_file(${qrc_xml_file} "<!DOCTYPE RCC>\n<RCC>${qrc_xml_body}</RCC>")
set(SYNTAX_DATA_QRC_FILES_STRING ${qrc_body})
configure_file(syntax-data.qrc.in ${qrc_file} @ONLY)

# compress xml files
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${compressed_xml_dir})

# generate the index file
add_custom_target(katesyntax DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax")
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax"
    COMMAND katehighlightingindexer
        "${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax"
        "${CMAKE_CURRENT_SOURCE_DIR}/schema/language.xsd"
        "${qrc_xml_file}"
        "${compressed_xml_dir}"
    DEPENDS katehighlightingindexer
        ${defs}
        ${qrc_file}
        ${CMAKE_CURRENT_SOURCE_DIR}/schema/language.xsd
)

# do we want syntax files bundled in the library?
if (QRC_SYNTAX)
    # generate the qrc file manually, to make dependencies on generated files work...
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp"
        COMMAND Qt6::rcc --name syntax_data -o "${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp" "${qrc_file}"
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax
    )
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp" PROPERTIES SKIP_AUTOMOC ON)

    target_sources(SyntaxHighlightingData PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/qrc_syntax-data.cpp)
else()
    add_dependencies(SyntaxHighlightingData katesyntax)
    # install the syntax files as normal files into the prefix
    install (FILES "${CMAKE_CURRENT_BINARY_DIR}/index.katesyntax" ${compressedDefs} DESTINATION ${KDE_INSTALL_DATADIR}/org.kde.syntax-highlighting/syntax-bundled)
endif()

# set PIC to allow use in static and shared libs
# this needs some more recent CMake than generally required
set_property(TARGET SyntaxHighlightingData PROPERTY POSITION_INDEPENDENT_CODE 1)
if(NOT ${CMAKE_VERSION} VERSION_LESS "3.13.0")
    target_link_libraries(SyntaxHighlightingData PRIVATE Qt6::Core)
endif()
