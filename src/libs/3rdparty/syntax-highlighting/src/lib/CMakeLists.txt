add_library(KF6SyntaxHighlighting)

set_target_properties(KF6SyntaxHighlighting PROPERTIES
    VERSION     ${KSYNTAXHIGHLIGHTING_VERSION}
    SOVERSION   ${KSYNTAXHIGHLIGHTING_SOVERSION}
    EXPORT_NAME SyntaxHighlighting
)

ecm_create_qm_loader(syntax_highlighting_QM_LOADER syntaxhighlighting6_qt)

target_sources(KF6SyntaxHighlighting PRIVATE
    abstracthighlighter.cpp
    context.cpp
    contextswitch.cpp
    definitiondownloader.cpp
    highlightingdata.cpp
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
ecm_qt_declare_logging_category(KF6SyntaxHighlighting
    HEADER ksyntaxhighlighting_logging.h
    IDENTIFIER KSyntaxHighlighting::Log
    CATEGORY_NAME kf.syntaxhighlighting
    OLD_CATEGORY_NAMES org.kde.ksyntaxhighlighting
    DESCRIPTION "Syntax Highlighting"
    EXPORT KSYNTAXHIGHLIGHTING
)

ecm_generate_export_header(KF6SyntaxHighlighting
    BASE_NAME KSyntaxHighlighting
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    USE_VERSION_HEADER
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS
    EXCLUDE_DEPRECATED_BEFORE_AND_AT ${EXCLUDE_DEPRECATED_BEFORE_AND_AT}
)

qt_extract_metatypes(KF6SyntaxHighlighting)
target_link_libraries(KF6SyntaxHighlighting
    PUBLIC
        Qt6::Gui
    PRIVATE
        Qt6::Network
)

set(Forwarding_Header_Names
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
)

ecm_generate_headers(CamelCase_HEADERS
    HEADER_NAMES ${Forwarding_Header_Names}
    REQUIRED_HEADERS SyntaxHighlighting_HEADERS
    SHARED_PREFIX KSyntaxHighlighting
)

target_include_directories(KF6SyntaxHighlighting
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KSyntaxHighlighting>"
)

install(TARGETS KF6SyntaxHighlighting EXPORT KF6SyntaxHighlightingTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    ${CamelCase_HEADERS}
    ${SyntaxHighlighting_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/ksyntaxhighlighting_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/KSyntaxHighlighting/KSyntaxHighlighting
)


ecm_generate_qdoc(KF6SyntaxHighlighting ksyntaxhighlighting.qdocconf)
