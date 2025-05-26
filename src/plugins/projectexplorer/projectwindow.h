// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fancymainwindow.h>
#include <utils/id.h>

#include <memory>

namespace Core { class OutputWindow; }

namespace ProjectExplorer::Internal {

class ProjectWindowPrivate;

class ProjectWindow : public Utils::FancyMainWindow
{
    friend class ProjectWindowPrivate;

public:
    ProjectWindow();
    ~ProjectWindow() override;

    void activateProjectPanel(Utils::Id panelId);
    void activateBuildSettings();
    void activateRunSettings();

    Core::OutputWindow *buildSystemOutput() const;

private:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

    void savePersistentSettings() const;
    void loadPersistentSettings();

    const std::unique_ptr<ProjectWindowPrivate> d;
};

} // namespace ProjectExplorer::Internal
