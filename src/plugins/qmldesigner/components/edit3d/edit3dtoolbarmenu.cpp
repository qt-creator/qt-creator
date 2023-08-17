// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dtoolbarmenu.h"

namespace QmlDesigner {

Edit3DToolbarMenu::Edit3DToolbarMenu(QWidget *parent)
    : QMenu(parent)
{
    setToolTipsVisible(true);
}

void Edit3DToolbarMenu::mouseReleaseEvent(QMouseEvent *e)
{
    QAction *action = activeAction();
    if (action && action->isEnabled() && action->isCheckable()) {
        // Prevent the menu from closing on click on any checkable item
        action->setEnabled(false);
        QMenu::mouseReleaseEvent(e);
        action->setEnabled(true);
        action->trigger();
    } else {
        QMenu::mouseReleaseEvent(e);
    }
}

} // namespace QmlDesigner
