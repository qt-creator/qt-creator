# This file is part of KDToolBox.
#
# SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
#
# SPDX-License-Identifier: MIT
#
find_package(
    Qt${QT_VERSION_MAJOR}
    ${QT_REQUIRED_VERSION}
    CONFIG
    REQUIRED
    Core
    Gui
    Widgets
)

include_directories(..)

set(ui_watchdog_example_SOURCES ../uiwatchdog.h main.cpp)

add_executable(ui_watchdog_example ${ui_watchdog_example_SOURCES})
target_link_libraries(ui_watchdog_example PUBLIC Qt::Core Qt::Gui Qt::Widgets)
