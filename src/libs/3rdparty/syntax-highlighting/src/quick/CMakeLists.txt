# SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
# SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

ecm_add_qml_module(kquicksyntaxhighlightingplugin URI "org.kde.syntaxhighlighting")

target_sources(kquicksyntaxhighlightingplugin PRIVATE
    kquicksyntaxhighlightingplugin.cpp
    kquicksyntaxhighlighter.cpp
    repositorywrapper.cpp
)
target_link_libraries(kquicksyntaxhighlightingplugin PRIVATE
    KF5SyntaxHighlighting
    Qt${QT_MAJOR_VERSION}::Quick
)

ecm_finalize_qml_module(kquicksyntaxhighlightingplugin DESTINATION ${KDE_INSTALL_QMLDIR})
