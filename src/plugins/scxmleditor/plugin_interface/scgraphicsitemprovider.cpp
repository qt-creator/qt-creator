// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scgraphicsitemprovider.h"
#include "idwarningitem.h"
#include "initialstateitem.h"
#include "initialwarningitem.h"
#include "scxmleditorconstants.h"
#include "stateitem.h"
#include "transitionitem.h"
#include "transitionwarningitem.h"

using namespace ScxmlEditor::PluginInterface;

SCGraphicsItemProvider::SCGraphicsItemProvider(QObject *parent)
    : GraphicsItemProvider(parent)
{
}

WarningItem *SCGraphicsItemProvider::createWarningItem(const QString &key, BaseItem *parentItem) const
{
    if (key == Constants::C_STATE_WARNING_ID && parentItem)
        return new IdWarningItem(parentItem);

    if (key == Constants::C_STATE_WARNING_TRANSITION && parentItem && parentItem->type() == TransitionType)
        return new TransitionWarningItem(static_cast<TransitionItem*>(parentItem));

    if (key == Constants::C_STATE_WARNING_INITIAL && parentItem && parentItem->type() == InitialStateType)
        return new InitialWarningItem(static_cast<InitialStateItem*>(parentItem));

    return nullptr;
}
