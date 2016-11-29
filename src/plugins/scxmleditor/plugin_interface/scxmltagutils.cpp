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

#include "scxmltagutils.h"
#include "scxmldocument.h"
#include "scxmleditorconstants.h"
#include "serializer.h"

#include <utils/qtcassert.h>

#include <QAction>
#include <QPointF>
#include <QRectF>

namespace ScxmlEditor {

namespace PluginInterface {

namespace TagUtils {

bool checkPaste(const QString &copiedTagTypes, const ScxmlTag *currentTag)
{
    if (copiedTagTypes.isEmpty() || !currentTag)
        return false;

    QVector<TagType> tagTypes;
    for (int i = 0; i < Finalize; ++i) {
        if (copiedTagTypes.contains(QLatin1String(scxml_tags[i].name)))
            tagTypes << TagType(i);
    }

    tagTypes.removeAll(InitialTransition);

    if (tagTypes.isEmpty())
        return false;

    QVector<TagType> childTags = allowedChildTypes(currentTag->tagType());
    foreach (const TagType &type, tagTypes) {
        if (!childTags.contains(type))
            return false;
    }

    return true;
}

void createChildMenu(const ScxmlTag *tag, QMenu *menu, bool addRemove)
{
    QTC_ASSERT(tag, return);

    initChildMenu(tag->tagType(), menu);

    QVariantMap data;
    data[Constants::C_SCXMLTAG_PARENTTAG] = tag->tagType();
    data[Constants::C_SCXMLTAG_ACTIONTYPE] = AddChild;

    if (tag->tagType() == UnknownTag) {
        data[Constants::C_SCXMLTAG_TAGTYPE] = UnknownTag;
        menu->addAction(ScxmlTag::tr("New Tag"))->setData(data);
    } else if (tag->tagType() == Metadata) {
        data[Constants::C_SCXMLTAG_TAGTYPE] = MetadataItem;
        menu->addAction(ScxmlTag::tr("Item"))->setData(data);
    } else {
        data[Constants::C_SCXMLTAG_PARENTTAG] = Metadata;
        data[Constants::C_SCXMLTAG_TAGTYPE] = MetadataItem;
        menu->addAction(ScxmlTag::tr("Metadata"))->setData(data);
    }

    if (addRemove) {
        menu->addSeparator();
        data[Constants::C_SCXMLTAG_ACTIONTYPE] = Remove;
        QAction *act = menu->addAction(ScxmlTag::tr("Remove"));
        act->setData(data);
        act->setEnabled(!tag->isRootTag());
    }
}

QVector<TagType> allowedChildTypes(TagType tagType)
{
    QVector<TagType> childTags;

    switch (tagType) {
    case Scxml:
        childTags << DataModel;
        childTags << Script;
        childTags << Initial;
        childTags << State;
        childTags << Parallel;
        childTags << Final;
        break;
    case State:
        childTags << Initial;
        childTags << Final;
        // FALL THROUGH
    case Parallel:
        childTags << OnEntry;
        childTags << OnExit;
        childTags << Transition;
        childTags << DataModel;
        childTags << Invoke;
        childTags << State;
        childTags << Parallel;
        childTags << History;
        break;
    case Initial:
    case History:
        childTags << Transition;
        break;
    case Final:
        childTags << OnEntry;
        childTags << OnExit;
        childTags << Donedata;
        break;
    case If:
        childTags << ElseIf;
        childTags << Else;
        // FALL THROUGH
    case Transition:
    case OnEntry:
    case OnExit:
    case ElseIf:
    case Else:
    case Foreach:
        // Executable content
        childTags << Raise;
        childTags << Send;
        childTags << Script;
        childTags << Assign;
        childTags << Cancel;
        childTags << Log;
        childTags << If;
        childTags << Foreach;
        break;
    case DataModel:
        childTags << Data;
        break;
    case Data:
        // PENDING
        break;
    case Assign:
        // PENDING
        break;
    case Content:
        // PENDING
        break;
    case Script:
        // PENDING
        break;
    case Invoke:
        childTags << Finalize;
        // FALL THROUGH
    case Donedata:
    case Send:
        childTags << Param;
        childTags << Content;
        break;
    default:
        break;
    }

    return childTags;
}

QVector<TagType> childTypes(TagType tagType)
{
    QVector<TagType> childTags;

    switch (tagType) {
    case Scxml:
        childTags << DataModel;
        childTags << Script;
        break;
    case State:
    case Parallel:
        childTags << OnEntry;
        childTags << OnExit;
        childTags << Transition;
        childTags << DataModel;
        childTags << Invoke;
        break;
    case Initial:
    case History:
        //childTags << Transition;
        break;
    case Final:
        childTags << OnEntry;
        childTags << OnExit;
        childTags << Donedata;
        break;
    case If:
        childTags << ElseIf;
        childTags << Else;
        // FALL THROUGH
    case Transition:
    case OnEntry:
    case OnExit:
    case ElseIf:
    case Else:
    case Foreach:
        // Executable content
        childTags << Raise;
        childTags << Send;
        childTags << Script;
        childTags << Assign;
        childTags << Cancel;
        childTags << Log;
        childTags << If;
        childTags << Foreach;
        break;
    case DataModel:
        childTags << Data;
        break;
    case Data:
        // PENDING
        break;
    case Assign:
        // PENDING
        break;
    case Content:
        // PENDING
        break;
    case Script:
        // PENDING
        break;
    case Invoke:
        childTags << Finalize;
        // FALL THROUGH
    case Donedata:
    case Send:
        childTags << Param;
        childTags << Content;
        break;
    default:
        break;
    }

    return childTags;
}

void initChildMenu(TagType tagType, QMenu *menu)
{
    menu->setTitle(QLatin1String(scxml_tags[tagType].name));

    QVector<TagType> childTags = childTypes(tagType);

    if (childTags.count() > 0) {
        for (int i = 0; i < childTags.count(); ++i) {
            if (childTags[i] == OnEntry || childTags[i] == OnExit)
                initChildMenu(childTags[i], menu->addMenu(QLatin1String(scxml_tags[childTags[i]].name)));
            else {
                QVariantMap data;
                data[Constants::C_SCXMLTAG_PARENTTAG] = tagType;
                data[Constants::C_SCXMLTAG_TAGTYPE] = childTags[i];
                data[Constants::C_SCXMLTAG_ACTIONTYPE] = AddChild;
                menu->addAction(QLatin1String(scxml_tags[childTags[i]].name))->setData(data);
            }
        }
    }
}

ScxmlTag *metadataTag(ScxmlTag *tag, const QString &key, bool blockUpdates)
{
    QTC_ASSERT(tag, return nullptr);

    ScxmlTag *info = nullptr;

    ScxmlDocument *document = tag->document();
    if (document) {
        ScxmlTag *metaData = tag->child("qt:metadata");
        if (!metaData) {
            metaData = new ScxmlTag(Metadata, document);
            if (!blockUpdates)
                document->addTag(tag, metaData);
            else
                tag->appendChild(metaData);
        }

        info = metaData->child(QString::fromLatin1("qt:%1").arg(key));
        if (!info) {
            info = new ScxmlTag(Metadata, document);
            info->setTagName(key);
            if (!blockUpdates)
                document->addTag(metaData, info);
            else
                metaData->appendChild(info);
        }
    }

    return info;
}

ScxmlTag *findChild(const ScxmlTag *tag, TagType childType)
{
    QTC_ASSERT(tag, return nullptr);

    for (int i = 0; i < tag->childCount(); ++i) {
        if (tag->child(i)->tagType() == childType)
            return tag->child(i);
    }

    return nullptr;
}

void findAllChildren(const ScxmlTag *tag, QVector<ScxmlTag*> &children)
{
    QTC_ASSERT(tag, return);

    for (int i = 0; i < tag->childCount(); ++i) {
        ScxmlTag *child = tag->child(i);
        children << child;
        findAllChildren(child, children);
    }
}

void findAllTransitionChildren(const ScxmlTag *tag, QVector<ScxmlTag*> &children)
{
    QTC_ASSERT(tag, return);

    for (int i = 0; i < tag->childCount(); ++i) {
        ScxmlTag *child = tag->child(i);
        TagType t = child->tagType();
        if (t == Transition || t == InitialTransition)
            children << child;
        else
            findAllTransitionChildren(child, children);
    }
}

void modifyPosition(ScxmlTag *tag, const QPointF &minPos, const QPointF &targetPos)
{
    QTC_ASSERT(tag, return);

    const QString sceneData = tag->editorInfo(Constants::C_SCXML_EDITORINFO_SCENEGEOMETRY);
    const QString localData = tag->editorInfo(Constants::C_SCXML_EDITORINFO_GEOMETRY);

    Serializer s;
    if (!localData.isEmpty() && !sceneData.isEmpty()) {
        QPointF localPos, scenePos;
        QRectF localRect, sceneRect;

        s.setData(sceneData);
        s.read(scenePos);
        s.read(sceneRect);

        s.clear();
        s.setData(localData);
        s.read(localPos);
        s.read(localRect);

        localPos = targetPos - localRect.topLeft() - (minPos - sceneRect.topLeft());

        s.clear();
        s.append(localPos);
        s.append(localRect);
        tag->document()->setEditorInfo(tag, Constants::C_SCXML_EDITORINFO_GEOMETRY, s.data());
    } else {
        s.append(targetPos);
        if (tag->tagType() == State || tag->tagType() == Parallel)
            s.append(QRectF(-60, -50, 120, 100));
        else if (tag->tagType() == Initial || tag->tagType() == Final || tag->tagType() == History)
            s.append(QRectF(-20, -20, 40, 40));
        else
            s.append(QRectF());
        tag->document()->setEditorInfo(tag, Constants::C_SCXML_EDITORINFO_GEOMETRY, s.data());
    }
}

} // namespace TagUtils
} // namespace PluginInterface
} // namespace ScxmlEditor
