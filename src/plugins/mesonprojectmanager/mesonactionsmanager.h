// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonprojectmanagertr.h"

#include <utils/parameteraction.h>

namespace MesonProjectManager {
namespace Internal {

class MesonActionsManager : public QObject
{
    Q_OBJECT
    Utils::ParameterAction buildTargetContextAction{
        ::MesonProjectManager::Tr::tr("Build"),
        ::MesonProjectManager::Tr::tr("Build \"%1\""),
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
