// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
            QString restoredInitial = parentTag->editorInfo("removedInitial");
            QString id = parentTag->attribute("initial");
            if (id.isEmpty())
                id = restoredInitial;

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

                if (!initialStateTag) {
                    parentTag->setEditorInfo("removedInitial", id);
                    parentTag->setAttribute("initial", QString());
                } else if (id == restoredInitial) {
                    parentTag->setAttribute("initial", id);
                    parentTag->setEditorInfo("removedInitial", QString());
                }
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

    for (QGraphicsItem *item : items) {
        if (item->type() >= StateType) {
            auto stateItem = static_cast<StateItem*>(item);
            if (stateItem)
                stateItem->setInitial(stateItem->tag() == initialStateTag);
        }
    }
}
