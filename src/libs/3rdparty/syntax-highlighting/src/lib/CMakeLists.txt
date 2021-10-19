add_library(KF5SyntaxHighlighting)

ecm_create_qm_loader(syntax_highlighting_QM_LOADER syntaxhighlighting5_qt)

target_sources(KF5SyntaxHighlighting PRIVATE
    abstracthighlighter.cpp
    context.cpp
    contextswitch.cpp
    definitiondownloader.cpp
    foldingregion.cpp
    format.cpp
    htmlhighlighter.cpp
    ansihighlighter.cpp
    keywordlist.cpp
    rule.cpp
    definition.cpp
    repository.cpp
    state.cpp
    syntaxhighlighter.cpp
    theme.cpp
    wildcardmatcher.cpp
    themedata.cpp
    worddelimiters.cpp
    ${syntax_highlighting_QM_LOADER}
    $<TARGET_OBJECTS:SyntaxHighlightingData>
)
ecm_qt_declare_logging_category(KF5SyntaxHighlighting
    HEADER ksyntaxhighlighting_logging.h
    IDENTIFIER KSyntaxHighlighting::Log
    CATEGORY_NAME kf.syntaxhighlighting
    OLD_CATEGORY_NAMES org.kde.ksyntaxhighlighting
    DESCRIPTION "Syntax Highlighting"
    EXPORT KSYNTAXHIGHLIGHTING
)

ecm_generate_export_header(KF5SyntaxHighlighting
    BASE_NAME KSyntaxHighlighting
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.87
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)
set_target_properties(KF5SyntaxHighlighting PROPERTIES
    VERSION ${SyntaxHighlighting_VERSION}
    SOVERSION ${SyntaxHighlighting_SOVERSION}
    EXPORT_NAME SyntaxHighlighting
)
target_include_directories(KF5SyntaxHighlighting INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5}/KSyntaxHighlighting;${KDE_INSTALL_INCLUDEDIR_KF5}>")
target_include_directories(KF5SyntaxHighlighting PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};>")
target_link_libraries(KF5SyntaxHighlighting
    PUBLIC
        Qt5::Gui
    PRIVATE
        Qt5::Network
)

ecm_generate_headers(SyntaxHighlighting_HEADERS
    HEADER_NAMES
        AbstractHighlighter
        Definition
        DefinitionDownloader
        FoldingRegion
        Format
        Repository
        State
        SyntaxHighlighter
        Theme
        WildcardMatcher
    REQUIRED_HEADERS SyntaxHighlighting_HEADERS
)

install(TARGETS KF5SyntaxHighlighting EXPORT KF5SyntaxHighlightingTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES
    ${SyntaxHighlighting_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/ksyntaxhighlighting_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KSyntaxHighlighting)

if(BUILD_QCH)
    ecm_add_qch(
        KF5SyntaxHighlighting_QCH
        NAME KSyntaxHighlighting
        BASE_NAME KF5SyntaxHighlighting
        VERSION ${KF_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${SyntaxHighlighting_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt5Core_QCH
            Qt5Gui_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        BLANK_MACROS
            KSYNTAXHIGHLIGHTING_EXPORT
            KSYNTAXHIGHLIGHTING_DEPRECATED
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
ecm_generate_pri_file(
    BASE_NAME KSyntaxHighlighting LIB_NAME
    KF5SyntaxHighlighting
    DEPS "gui"
    FILENAME_VAR PRI_FILENAME
    INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5}/KSyntaxHighlighting
)
install(FILES ${PRI_FILENAME} DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
