add_executable(kate-syntax-highlighter kate-syntax-highlighter.cpp)
ecm_mark_nongui_executable(kate-syntax-highlighter)
target_link_libraries(kate-syntax-highlighter KF5SyntaxHighlighting)

install(TARGETS kate-syntax-highlighter ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
