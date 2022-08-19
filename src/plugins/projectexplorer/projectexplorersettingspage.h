// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_projectexplorersettingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace ProjectExplorer {
namespace Internal {

class ProjectExplorerSettings;
class ProjectExplorerSettingsWidget;

class ProjectExplorerSettingsPage : public Core::IOptionsPage
{
public:
    ProjectExplorerSettingsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QPointer<ProjectExplorerSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace ProjectExplorer
