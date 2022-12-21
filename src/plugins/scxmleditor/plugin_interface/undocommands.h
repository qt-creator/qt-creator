// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmldocument.h"
#include "scxmltag.h"
#include <QUndoCommand>

namespace ScxmlEditor {

namespace PluginInterface {

class BaseUndoCommand : public QUndoCommand
{
public:
    BaseUndoCommand(ScxmlDocument *doc, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

protected:
    virtual void doUndo() = 0;
    virtual void doRedo() = 0;

private:
    ScxmlDocument *m_doc;
    bool m_firstTime = true;
};

class AddRemoveTagsBeginCommand : public BaseUndoCommand
{
public:
    AddRemoveTagsBeginCommand(ScxmlDocument *doc, ScxmlTag *tag, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

private:
    ScxmlDocument *m_document;
    ScxmlTag *m_tag;
};

class AddRemoveTagsEndCommand : public BaseUndoCommand
{
public:
    AddRemoveTagsEndCommand(ScxmlDocument *doc, ScxmlTag *tag, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

private:
    ScxmlDocument *m_document;
    ScxmlTag *m_tag;
};

/**
 * @brief The AddRemoveTagCommand class provides the undo-command to add/remove tag to/from SCXML-document.
 */
class AddRemoveTagCommand : public BaseUndoCommand
{
public:
    AddRemoveTagCommand(ScxmlDocument *doc, ScxmlTag *parentTag, ScxmlTag *tag, ScxmlDocument::TagChange change, QUndoCommand *parent = nullptr);
    ~AddRemoveTagCommand() override;

    void doUndo() override;
    void doRedo() override;

private:
    void doAction(bool add);
    ScxmlDocument *m_document;
    QPointer<ScxmlTag> m_tag;
    QPointer<ScxmlTag> m_parentTag;
    ScxmlDocument::TagChange m_change;
};

/**
 * @brief The SetAttributeCommand class provides the undo-command to set attribute-value for the given ScxmlTag.
 */
class SetAttributeCommand : public BaseUndoCommand
{
public:
    SetAttributeCommand(ScxmlDocument *doc, ScxmlTag *tag, const QString &key, const QString &value, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

    bool mergeWith(const QUndoCommand *other) override;
    int id() const override;

private:
    void doAction(const QString &key, const QString &value);
    ScxmlDocument *m_document;
    QPointer<ScxmlTag> m_tag;
    QString m_key;
    QString m_value;
    QString m_oldValue;
};

/**
 * @brief The SetContentCommand class
 */
class SetContentCommand : public BaseUndoCommand
{
public:
    SetContentCommand(ScxmlDocument *doc, ScxmlTag *tag, const QString &content, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

    bool mergeWith(const QUndoCommand *other) override;
    int id() const override;

private:
    void doAction(const QString &content);
    ScxmlDocument *m_document;
    QPointer<ScxmlTag> m_tag;
    QString m_content;
    QString m_oldContent;
};

/**
 * @brief The SetEditorInfoCommand class provides the undo-command to set editorinfo-value for the given ScxmlTag.
 */
class SetEditorInfoCommand : public BaseUndoCommand
{
public:
    SetEditorInfoCommand(ScxmlDocument *doc, ScxmlTag *tag, const QString &key, const QString &value, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

    bool mergeWith(const QUndoCommand *other) override;
    int id() const override;

private:
    void doAction(const QString &key, const QString &value);
    ScxmlDocument *m_document;
    QPointer<ScxmlTag> m_tag;
    QString m_key;
    QString m_value;
    QString m_oldValue;
};

/**
 * @brief The ChangeParentCommand class provides the undo-commant to change parent of the ScxmlTag.
 */
class ChangeParentCommand : public BaseUndoCommand
{
public:
    ChangeParentCommand(ScxmlDocument *doc, ScxmlTag *tag, ScxmlTag *newParentTag, int tagIndex, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

private:
    void doAction(ScxmlTag *oldParent, ScxmlTag *newParent);

    ScxmlDocument *m_document;
    QPointer<ScxmlTag> m_tag;
    QPointer<ScxmlTag> m_newParentTag;
    QPointer<ScxmlTag> m_oldParentTag;
    int m_tagIndex;
};

/**
 * @brief The ChangeOrderCommand class
 */
class ChangeOrderCommand : public BaseUndoCommand
{
public:
    ChangeOrderCommand(ScxmlDocument *doc, ScxmlTag *tag, ScxmlTag *parentTag, int newPos, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

private:
    void doAction(int newPos);

    ScxmlDocument *m_document;
    QPointer<ScxmlTag> m_tag;
    QPointer<ScxmlTag> m_parentTag;
    int m_oldPos;
    int m_newPos;
};

/**
 * @brief The ChangeNameSpaceCommand class
 */
class ChangeFullNameSpaceCommand : public BaseUndoCommand
{
public:
    ChangeFullNameSpaceCommand(ScxmlDocument *doc, ScxmlTag *tag, bool state, QUndoCommand *parent = nullptr);

    void doUndo() override;
    void doRedo() override;

private:
    void doAction(bool state);
    void makeIdMap(ScxmlTag *tag, QHash<QString, QString> &map, bool use);
    void updateNameSpace(ScxmlTag *tag, const QHash<QString, QString> &map);

    ScxmlDocument *m_document;
    QPointer<ScxmlTag> m_rootTag;
    bool m_oldState;
    bool m_newState;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
