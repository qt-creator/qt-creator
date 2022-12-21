// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmltag.h"
#include <QMenu>
#include <QVector>

namespace ScxmlEditor {

namespace PluginInterface {

namespace TagUtils {

enum MenuAction {
    AddChild = 0,
    SetAsInitial,
    Relayout,
    ZoomToState,
    RemovePoint,
    Remove
};

bool checkPaste(const QString &copiedTagType, const ScxmlTag *currentTag);
QVector<TagType> allowedChildTypes(TagType tagType);
QVector<TagType> childTypes(TagType type);
void createChildMenu(const ScxmlTag *tag, QMenu *menu, bool addRemove = true);
void initChildMenu(TagType tagType, QMenu *menu);
ScxmlTag *findChild(const ScxmlTag *tag, TagType childType);
ScxmlTag *metadataTag(ScxmlTag *tag, const QString &tagname, bool blockUpdates = false);
void findAllChildren(const ScxmlTag *tag, QVector<ScxmlTag*> &children);
void findAllTransitionChildren(const ScxmlTag *tag, QVector<ScxmlTag*> &children);
void modifyPosition(ScxmlTag *tag, const QPointF &minPos, const QPointF &targetPos);

} // namespace TagUtils
} // namespace PluginInterface
} // namespace ScxmlEditor
