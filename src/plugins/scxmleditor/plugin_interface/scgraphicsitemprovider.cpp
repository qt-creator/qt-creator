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
