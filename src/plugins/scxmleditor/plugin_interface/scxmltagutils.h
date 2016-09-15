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
