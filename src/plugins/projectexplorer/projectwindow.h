// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/fancymainwindow.h>
#include <utils/id.h>

#include <memory>

namespace Core {
class OutputWindow;
}

namespace ProjectExplorer {
namespace Internal {

enum {
    ContextMenuItemAdderRole // To augment a context menu, data has a QMenu*
        = Qt::UserRole + 1,

    ProjectDisplayNameRole,       // Shown in the project selection combobox
    ItemActivatedDirectlyRole,    // This item got activated through user interaction and
                                  // is now responsible for the central widget.
    ItemActivatedFromBelowRole,   // A subitem gots activated and gives us the opportunity to adjust
    ItemActivatedFromAboveRole,   // A parent item gots activated and makes us its active child.
    ItemDeactivatedFromBelowRole, // A subitem got deactivated and gives us the opportunity to adjust
    ItemUpdatedFromBelowRole,     // A subitem got updated, re-expansion is necessary.
    ActiveItemRole,               // The index of the currently selected item in the tree view
    KitIdRole,                    // The kit id in case the item is associated with a kit.
    PanelWidgetRole               // This item's widget to be shown as central widget.
};

class ProjectWindowPrivate;

class ProjectWindow : public Utils::FancyMainWindow
{
    friend class ProjectWindowPrivate;
    Q_OBJECT

public:
    ProjectWindow();
    ~ProjectWindow() override;

    void activateProjectPanel(Utils::Id panelId);

    Core::OutputWindow *buildSystemOutput() const;

private:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

    void savePersistentSettings() const;
    void loadPersistentSettings();

    const std::unique_ptr<ProjectWindowPrivate> d;
};

} // namespace Internal
} // namespace ProjectExplorer
