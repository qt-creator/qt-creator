// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildoptionsmodel.h"

#include <projectexplorer/namedwidget.h>

#include <utils/categorysortfiltermodel.h>
#include <utils/progressindicator.h>

#include <QTimer>

namespace MesonProjectManager::Internal {

class MesonBuildConfiguration;

class MesonBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    explicit MesonBuildSettingsWidget(MesonBuildConfiguration *buildCfg);
    ~MesonBuildSettingsWidget();

private:
    BuidOptionsModel m_optionsModel;
    Utils::CategorySortFilterModel m_optionsFilter;
    Utils::ProgressIndicator m_progressIndicator;
    QTimer m_showProgressTimer;
};

} // MesonProjectManager::Internal
