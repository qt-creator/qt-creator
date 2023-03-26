// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "undocommands.h"

using namespace ScxmlEditor::PluginInterface;

BaseUndoCommand::BaseUndoCommand(ScxmlDocument *doc, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_doc(doc)
{
}

void BaseUndoCommand::undo()
{
    m_doc->setUndoRedoRunning(true);
    doUndo();
    m_doc->setUndoRedoRunning(false);
}

void BaseUndoCommand::redo()
{
    m_doc->setUndoRedoRunning(!m_firstTime);
    doRedo();
    if (m_firstTime)
        m_firstTime = false;
    m_doc->setUndoRedoRunning(false);
}

// AddRemoveTagsBeginCommand
AddRemoveTagsBeginCommand::AddRemoveTagsBeginCommand(ScxmlDocument *doc, ScxmlTag *tag, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
{
}

void AddRemoveTagsBeginCommand::doUndo()
{
    emit m_document->endTagChange(ScxmlDocument::TagRemoveTags, m_tag, m_tag->index());
}
void AddRemoveTagsBeginCommand::doRedo()
{
    emit m_document->beginTagChange(ScxmlDocument::TagAddTags, m_tag, m_tag->index());
}

// AddRemoveTagsEndCommand
AddRemoveTagsEndCommand::AddRemoveTagsEndCommand(ScxmlDocument *doc, ScxmlTag *tag, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
{
}

void AddRemoveTagsEndCommand::doUndo()
{
    emit m_document->beginTagChange(ScxmlDocument::TagRemoveTags, m_tag, m_tag->index());
}
void AddRemoveTagsEndCommand::doRedo()
{
    emit m_document->endTagChange(ScxmlDocument::TagAddTags, m_tag, m_tag->index());
}

// AddRemoveTagCommand
AddRemoveTagCommand::AddRemoveTagCommand(ScxmlDocument *doc, ScxmlTag *parentTag, ScxmlTag *tag, ScxmlDocument::TagChange change, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
    , m_parentTag(parentTag)
    , m_change(change)
{
    m_tag->setDocument(m_document);
}

AddRemoveTagCommand::~AddRemoveTagCommand()
{
    // If type was ADD, then delete tag
    if (m_change == ScxmlDocument::TagAddChild)
        delete m_tag;
}

void AddRemoveTagCommand::doAction(bool add)
{
    if (add) {
        int r = m_parentTag->childIndex(m_tag);
        if (r < 0)
            r = m_parentTag->childCount();
        emit m_document->beginTagChange(ScxmlDocument::TagAddChild, m_parentTag, QVariant(r));
        m_parentTag->appendChild(m_tag);
        emit m_document->endTagChange(ScxmlDocument::TagAddChild, m_parentTag, QVariant(r));
    } else {
        int r = m_parentTag->childIndex(m_tag);
        if (r >= 0) {
            emit m_document->beginTagChange(ScxmlDocument::TagRemoveChild, m_parentTag, QVariant(r));
            m_parentTag->removeChild(m_tag);
            emit m_document->endTagChange(ScxmlDocument::TagRemoveChild, m_parentTag, QVariant(r));
        }
    }
}

void AddRemoveTagCommand::doUndo()
{
    doAction(m_change != ScxmlDocument::TagAddChild);
}
void AddRemoveTagCommand::doRedo()
{
    doAction(m_change == ScxmlDocument::TagAddChild);
}

// SetAttributeCommand
SetAttributeCommand::SetAttributeCommand(ScxmlDocument *doc, ScxmlTag *tag, const QString &key, const QString &value, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
    , m_key(key)
    , m_value(value)
{
    m_oldValue = m_tag->attribute(m_key);
}

void SetAttributeCommand::doAction(const QString &key, const QString &value)
{
    emit m_document->beginTagChange(ScxmlDocument::TagAttributesChanged, m_tag, m_tag->attribute(key));

    QString newValue = value;
    if (m_tag->tagType() == Transition && key == "target" && !m_document->tagForId(value))
        newValue = QString();
    m_tag->setAttribute(key, newValue);
    emit m_document->endTagChange(ScxmlDocument::TagAttributesChanged, m_tag, newValue);
}

int SetAttributeCommand::id() const
{
    return (int)ScxmlDocument::TagAttributesChanged;
}
bool SetAttributeCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() == id()) {
        QString key = static_cast<const SetAttributeCommand*>(other)->m_key;
        auto tag = static_cast<const SetAttributeCommand*>(other)->m_tag;
        if (tag == m_tag && key == m_key) {
            m_value = static_cast<const SetAttributeCommand*>(other)->m_value;
            return true;
        }
    }

    return false;
}

void SetAttributeCommand::doUndo()
{
    doAction(m_key, m_oldValue);
}
void SetAttributeCommand::doRedo()
{
    doAction(m_key, m_value);
}

// SetContentCommand
SetContentCommand::SetContentCommand(ScxmlDocument *doc, ScxmlTag *tag, const QString &content, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
    , m_content(content)
{
    m_oldContent = m_tag->content();
}

void SetContentCommand::doAction(const QString &content)
{
    emit m_document->beginTagChange(ScxmlDocument::TagContentChanged, m_tag, m_tag->content());
    m_tag->setContent(content);
    emit m_document->endTagChange(ScxmlDocument::TagContentChanged, m_tag, content);
}

int SetContentCommand::id() const
{
    return (int)ScxmlDocument::TagContentChanged;
}
bool SetContentCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() == id()) {
        auto tag = static_cast<const SetContentCommand*>(other)->m_tag;
        if (tag == m_tag) {
            m_content = static_cast<const SetContentCommand*>(other)->m_content;
            return true;
        }
    }

    return false;
}

void SetContentCommand::doUndo()
{
    doAction(m_oldContent);
}
void SetContentCommand::doRedo()
{
    doAction(m_content);
}

// SetEditorInfoCommand
SetEditorInfoCommand::SetEditorInfoCommand(ScxmlDocument *doc, ScxmlTag *tag, const QString &key, const QString &value, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
    , m_key(key)
    , m_value(value)
{
    m_oldValue = m_tag->editorInfo(m_key);
}

void SetEditorInfoCommand::doAction(const QString &key, const QString &value)
{
    emit m_document->beginTagChange(ScxmlDocument::TagEditorInfoChanged, m_tag, m_tag->editorInfo(key));
    m_tag->setEditorInfo(key, value);
    emit m_document->endTagChange(ScxmlDocument::TagEditorInfoChanged, m_tag, value);
}

int SetEditorInfoCommand::id() const
{
    return (int)ScxmlDocument::TagEditorInfoChanged;
}
bool SetEditorInfoCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() == id()) {
        QString key = static_cast<const SetEditorInfoCommand*>(other)->m_key;
        auto tag = static_cast<const SetEditorInfoCommand*>(other)->m_tag;
        if (tag == m_tag && key == m_key) {
            m_value = static_cast<const SetEditorInfoCommand*>(other)->m_value;
            return true;
        }
    }

    return false;
}

void SetEditorInfoCommand::doUndo()
{
    doAction(m_key, m_oldValue);
}
void SetEditorInfoCommand::doRedo()
{
    doAction(m_key, m_value);
}

// ChangeParentCommand
ChangeParentCommand::ChangeParentCommand(ScxmlDocument *doc, ScxmlTag *tag, ScxmlTag *newParentTag, int tagIndex, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
    , m_newParentTag(newParentTag)
    , m_tagIndex(tagIndex)
{
    m_oldParentTag = m_tag->parentTag();
}

void ChangeParentCommand::doAction(ScxmlTag *oldParent, ScxmlTag *newParent)
{
    emit m_document->beginTagChange(ScxmlDocument::TagChangeParent, m_tag, m_tag->index());

    int r = oldParent->childIndex(m_tag);
    emit m_document->beginTagChange(ScxmlDocument::TagChangeParentRemoveChild, oldParent, QVariant(r));
    oldParent->removeChild(m_tag);
    emit m_document->endTagChange(ScxmlDocument::TagChangeParentRemoveChild, oldParent, QVariant(r));

    r = newParent->childCount();
    emit m_document->beginTagChange(ScxmlDocument::TagChangeParentAddChild, newParent, QVariant(r));
    newParent->insertChild(m_tagIndex, m_tag);
    emit m_document->endTagChange(ScxmlDocument::TagChangeParentAddChild, newParent, QVariant(r));

    emit m_document->endTagChange(ScxmlDocument::TagChangeParent, m_tag, m_tag->index());
}

void ChangeParentCommand::doUndo()
{
    doAction(m_newParentTag, m_oldParentTag);
}
void ChangeParentCommand::doRedo()
{
    doAction(m_oldParentTag, m_newParentTag);
}

ChangeOrderCommand::ChangeOrderCommand(ScxmlDocument *doc, ScxmlTag *tag, ScxmlTag *parentTag, int newPos, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_tag(tag)
    , m_parentTag(parentTag)
    , m_newPos(newPos)
{
    m_oldPos = m_tag->index();
}

void ChangeOrderCommand::doAction(int newPos)
{
    emit m_document->beginTagChange(ScxmlDocument::TagChangeOrder, m_tag, newPos);
    m_parentTag->moveChild(m_tag->index(), newPos);
    emit m_document->endTagChange(ScxmlDocument::TagChangeOrder, m_tag, m_tag->index());
}

void ChangeOrderCommand::doUndo()
{
    doAction(m_oldPos);
}
void ChangeOrderCommand::doRedo()
{
    doAction(m_newPos);
}

ChangeFullNameSpaceCommand::ChangeFullNameSpaceCommand(ScxmlDocument *doc, ScxmlTag *tag, bool state, QUndoCommand *parent)
    : BaseUndoCommand(doc, parent)
    , m_document(doc)
    , m_rootTag(tag)
    , m_newState(state)
{
    m_oldState = !state;
}

void ChangeFullNameSpaceCommand::makeIdMap(ScxmlTag *tag, QHash<QString, QString> &map, bool use)
{
    switch (tag->tagType()) {
    case History:
    case Final:
    case State:
    case Parallel: {
        QString strId = tag->attribute("id");
        QString strIdWithNS = QString::fromLatin1("%1%2").arg(tag->stateNameSpace()).arg(strId);
        map[use ? strId : strIdWithNS] = use ? strIdWithNS : strId;
        break;
    }
    default:
        break;
    }

    const QVector<ScxmlTag *> children = tag->allChildren();
    for (ScxmlTag *child : children) {
        makeIdMap(child, map, use);
    }
}

void ChangeFullNameSpaceCommand::updateNameSpace(ScxmlTag *tag, const QHash<QString, QString> &map)
{
    QString name;
    switch (tag->tagType()) {
    case Scxml:
    case State:
        name = "initial";
        break;
    case Transition:
        name = "target";
        break;
    default:
        break;
    }

    if (!name.isEmpty()) {
        QString attr = tag->attribute(name);
        if (map.contains(attr))
            tag->setAttribute(name, map[attr]);
    }

    const QVector<ScxmlTag *> children = tag->allChildren();
    for (ScxmlTag *child : children) {
        updateNameSpace(child, map);
    }
}

void ChangeFullNameSpaceCommand::doAction(bool newState)
{
    emit m_document->beginTagChange(ScxmlDocument::TagChangeFullNameSpace, m_rootTag, newState);

    QHash<QString, QString> keyMap;
    makeIdMap(m_rootTag, keyMap, newState);
    updateNameSpace(m_rootTag, keyMap);
    m_document->m_useFullNameSpace = newState;

    emit m_document->endTagChange(ScxmlDocument::TagChangeFullNameSpace, m_rootTag, newState);
}

void ChangeFullNameSpaceCommand::doUndo()
{
    doAction(m_oldState);
}
void ChangeFullNameSpaceCommand::doRedo()
{
    doAction(m_newState);
}
