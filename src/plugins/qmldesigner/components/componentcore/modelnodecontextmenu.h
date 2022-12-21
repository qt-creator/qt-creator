// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPoint>
#include <QCoreApplication>

#include <abstractview.h>
#include "selectioncontext.h"

namespace QmlDesigner {

class ModelNodeContextMenu
{
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::ModelNodeContextMenu)
public:
    ModelNodeContextMenu(AbstractView *view);
    void execute(const QPoint &pos, bool selectionMenu);
    void setScenePos(const QPoint &pos);

    static void showContextMenu(AbstractView *view, const QPoint &globalPosition, const QPoint &scenePosition, bool showSelection);

private:
    QPoint m_scenePos;
    SelectionContext m_selectionContext;
};

} // namespace QmlDesigner
