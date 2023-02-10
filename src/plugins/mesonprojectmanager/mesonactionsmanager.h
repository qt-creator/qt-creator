// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/parameteraction.h>

#include <QCoreApplication>

namespace MesonProjectManager {
namespace Internal {

class MesonActionsManager : public QObject
{
    Q_OBJECT
    Utils::ParameterAction buildTargetContextAction{
        QCoreApplication::translate("QtC::MesonProjectManager", "Build"),
        QCoreApplication::translate("QtC::MesonProjectManager", "Build \"%1\""),
        Utils::ParameterAction::AlwaysEnabled /*handled manually*/
    };
    QAction configureActionMenu;
    QAction configureActionContextMenu;
    void configureCurrentProject();
    void updateContextActions();

public:
    MesonActionsManager();
};

} // namespace Internal
} // namespace MesonProjectManager
