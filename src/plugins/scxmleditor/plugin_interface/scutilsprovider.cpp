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

#include "scutilsprovider.h"
#include "mytypes.h"
#include "scxmltag.h"
#include "stateitem.h"

using namespace ScxmlEditor::PluginInterface;

SCUtilsProvider::SCUtilsProvider(QObject *parent)
    : UtilsProvider(parent)
{
}

void SCUtilsProvider::checkInitialState(const QList<QGraphicsItem*> &items, ScxmlTag *parentTag)
{
    ScxmlTag *initialStateTag = nullptr;

    if (parentTag) {

        // 1. If we have initial-state, we must use it as init-state
        if (parentTag->hasChild(Initial)) {
            parentTag->setAttribute("initial", QString());
        } else {
            QString id = parentTag->attribute("initial");

            // 2. If no initial-state available, try to find state with initial-attribute
            if (!id.isEmpty()) {
                // Find state with id
                for (int i = 0; i < parentTag->childCount(); ++i) {
                    ScxmlTag *child = parentTag->child(i);
                    if ((child->tagType() == State || child->tagType() == Parallel)
                        && child->attribute("id", true) == id) {
                        initialStateTag = child;
                        break;
                    }
                }

                if (!initialStateTag)
                    parentTag->setAttribute("initial", QString());
            }

            // 3. If we still cant find initial-state, we must select first
            if (!initialStateTag) {
                // Search first state
                for (int i = 0; i < parentTag->childCount(); ++i) {
                    ScxmlTag *child = parentTag->child(i);
                    if (child->tagType() == State || child->tagType() == Parallel) {
                        initialStateTag = child;
                        break;
                    }
                }
            }
        }
    }

    foreach (QGraphicsItem *item, items) {
        if (item->type() >= StateType) {
            auto stateItem = static_cast<StateItem*>(item);
            if (stateItem)
                stateItem->setInitial(stateItem->tag() == initialStateTag);
        }
    }
}
