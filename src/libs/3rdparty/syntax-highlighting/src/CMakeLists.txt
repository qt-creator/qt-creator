add_subdirectory(indexer)
if(TARGET Qt5::Gui)
    add_subdirectory(lib)
    add_subdirectory(cli)
endif()

ecm_qt_install_logging_categories(
    EXPORT KSYNTAXHIGHLIGHTING
    FILE ksyntaxhighlighting.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
