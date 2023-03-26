// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utilitypanelcontroller.h"

#include <QDebug>
#include <QWidget>

namespace QmlDesigner {

UtilityPanelController::UtilityPanelController(QObject* parent):
        QObject(parent)
{
}

UtilityPanelController::~UtilityPanelController() = default;

QWidget* UtilityPanelController::widget()
{
    return contentWidget();
}

} // namespace QmlDesigner
