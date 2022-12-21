// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineutils.h"

#include <QEvent>

namespace QmlDesigner {

namespace TimelineUtils {

DisableContextMenu::DisableContextMenu(QObject *parent)
    : QObject(parent)
{}

bool DisableContextMenu::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ContextMenu)
        return true;

    return QObject::eventFilter(watched, event);
}

} // End namespace TimelineUtils.

} // End namespace QmlDesigner.
