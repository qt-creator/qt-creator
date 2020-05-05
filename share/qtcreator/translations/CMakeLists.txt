set(languages cs da de fr hr ja pl ru sl uk zh_CN zh_TW)
set(bad_languages hu) # Fix these before including them in languages!

find_package(PythonInterp)

set(json_wizards_h "")
set(custom_wizards_h "")
set(externaltools_h "")
set(snippets_h "")

if (NOT PYTHONINTERP_FOUND OR NOT PYTHON_VERSION_STRING VERSION_GREATER_EQUAL "3.0.0")
  message(WARNING "No python3 interpreter found, skipping extraction of data from XML and JSON files.\n *** Please pass -DPYTHON_EXECUTABLE=/path/to/python3 to cmake.")
else()
  set(json_wizards_h "${CMAKE_CURRENT_BINARY_DIR}/jsonwizards_tr.h")
  add_custom_command(OUTPUT "${json_wizards_h}"
    COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/extract-jsonwizards.py"
      "${PROJECT_SOURCE_DIR}/share/qtcreator/templates/wizards" "${json_wizards_h}"
    COMMENT Generate translation data from JSON wizards
    VERBATIM)

  set(custom_wizards_h "${CMAKE_CURRENT_BINARY_DIR}/customwizards_tr.h")
  add_custom_command(OUTPUT "${custom_wizards_h}"
    COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/extract-customwizards.py"
      "${PROJECT_SOURCE_DIR}/share/qtcreator/templates/wizards" "${custom_wizards_h}"
    COMMENT Generate translation data from XML wizards
    VERBATIM)

  set(externaltools_h "${CMAKE_CURRENT_BINARY_DIR}/externaltools_tr.h")
  add_custom_command(OUTPUT "${externaltools_h}"
    COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/extract-externaltools.py"
      "${PROJECT_SOURCE_DIR}/src/share/qtcreator/externaltools" "${externaltools_h}"
    COMMENT Generate translation data from external tools definitions
    VERBATIM)

  set(snippets_h "${CMAKE_CURRENT_BINARY_DIR}/snippets_tr.h")
  add_custom_command(OUTPUT "${snippets_h}"
    COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/extract-snippets.py"
      "${PROJECT_SOURCE_DIR}/share/qtcreator/snippets" "${snippets_h}"
    COMMENT Generate translation data from snippets definitions
    VERBATIM)
endif()

add_translation_targets(qtcreator
  LANGUAGES ${languages}
  OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}/translations"
  INSTALL_DESTINATION "${IDE_DATA_PATH}/translations"
  TARGETS "${__QTC_LIBRARIES}" "${__QTC_PLUGINS}"
  SOURCES "${json_wizards_h}" "${custom_wizards_h}" "${externaltools_h}" "${snippets_h}")
