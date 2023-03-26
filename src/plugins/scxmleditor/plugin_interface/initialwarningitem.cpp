// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "initialstateitem.h"
#include "initialwarningitem.h"
#include "sceneutils.h"
#include "scxmleditortr.h"

using namespace ScxmlEditor::PluginInterface;

InitialWarningItem::InitialWarningItem(InitialStateItem *parent)
    : WarningItem(parent)
    , m_parentItem(parent)
{
    setSeverity(OutputPane::Warning::ErrorType);
    setTypeName(Tr::tr("Initial"));
    setDescription(Tr::tr("One level can contain only one initial state."));
    setReason(Tr::tr("Too many initial states at the same level."));
}

void InitialWarningItem::updatePos()
{
    setPos(m_parentItem->boundingRect().topLeft());
}

void InitialWarningItem::check()
{
    if (m_parentItem)
        setWarningActive(SceneUtils::hasSiblingStates(m_parentItem));
}
