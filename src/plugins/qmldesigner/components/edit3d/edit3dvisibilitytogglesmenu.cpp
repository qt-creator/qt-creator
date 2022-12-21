// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dvisibilitytogglesmenu.h"

namespace QmlDesigner {

Edit3DVisibilityTogglesMenu::Edit3DVisibilityTogglesMenu(QWidget *parent) :
    QMenu(parent)
{
    setToolTipsVisible(true);
}

void Edit3DVisibilityTogglesMenu::mouseReleaseEvent(QMouseEvent *e)
{
    QAction *action = activeAction();
    if (action && action->isEnabled()) {
        // Prevent the menu from closing on click on any item
        action->setEnabled(false);
        QMenu::mouseReleaseEvent(e);
        action->setEnabled(true);
        action->trigger();
    } else {
        QMenu::mouseReleaseEvent(e);
    }
}

} // namespace QmlDesigner
