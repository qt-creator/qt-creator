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

#include "initialwarningitem.h"
#include "initialstateitem.h"
#include "sceneutils.h"

using namespace ScxmlEditor::PluginInterface;

InitialWarningItem::InitialWarningItem(InitialStateItem *parent)
    : WarningItem(parent)
    , m_parentItem(parent)
{
    setSeverity(OutputPane::Warning::ErrorType);
    setTypeName(tr("Initial"));
    setDescription(tr("It is possible to have max 1 initial-state in the same level."));
    setReason(tr("Too many initial states in the same level"));
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
