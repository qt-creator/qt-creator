/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_export.h"

#include <utils/fancymainwindow.h>

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
    Q_OBJECT

public:
    ProjectWindow();
    ~ProjectWindow();

private:
    ProjectWindowPrivate *d;
};

} // namespace Internal
} // namespace ProjectExplorer
