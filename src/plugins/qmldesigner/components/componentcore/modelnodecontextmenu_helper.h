// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractaction.h"
#include "abstractactiongroup.h"
#include "utils3d.h"

#include <bindingproperty.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <modelutils.h>
#include <nodemetainfo.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>

#include <utils/proxyaction.h>

#include <QAction>
#include <QMenu>

#include <functional>

namespace QmlDesigner {

using SelectionContextPredicate = std::function<bool (const SelectionContext&)>;
using SelectionContextOperation = std::function<void (const SelectionContext&)>;

namespace SelectionContextHelpers {

bool contains(const QmlItemNode &node, const QPointF &position);

} // namespace SelectionContextHelpers

namespace SelectionContextFunctors {

QMLDESIGNERCOMPONENTS_EXPORT bool always(const SelectionContext &);

bool inBaseState(const SelectionContext &selectionState);

bool isFileComponent(const SelectionContext &selectionContext);

bool singleSelection(const SelectionContext &selectionState);

bool addMouseAreaFillCheck(const SelectionContext &selectionContext);

bool isModelOrMaterial(const SelectionContext &selectionState);

bool enableAddToContentLib(const SelectionContext &selectionState);

bool are3DNodes(const SelectionContext &selectionState);

bool hasEditableMaterial(const SelectionContext &selectionState);

bool selectionEnabled(const SelectionContext &selectionState);

bool selectionNotEmpty(const SelectionContext &selectionState);

bool selectionNot2D3DMix(const SelectionContext &selectionState);

bool singleSelectionNotRoot(const SelectionContext &selectionState);

bool singleSelectionView3D(const SelectionContext &selectionState);

bool singleSelectionEffectComposer(const SelectionContext &selectionState);

bool selectionHasProperty(const SelectionContext &selectionState, const char *property);

bool selectionHasSlot(const SelectionContext &selectionState, const char *property);

bool singleSelectedItem(const SelectionContext &selectionState);

bool selectionHasSameParent(const SelectionContext &selectionState);
bool selectionIsEditableComponent(const SelectionContext &selectionState);
bool singleSelectionItemIsAnchored(const SelectionContext &selectionState);
bool singleSelectionItemIsNotAnchored(const SelectionContext &selectionState);
bool selectionIsImported3DAsset(const SelectionContext &selectionState);
bool singleSelectionItemHasAnchor(const SelectionContext &selectionState, AnchorLineType anchor);

} // namespace SelectionStateFunctors

class ActionTemplate : public DefaultAction
{

public:
    ActionTemplate(const QByteArray &id, const QString &description, SelectionContextOperation action)
        : DefaultAction(description), m_action(action), m_id(id)
    { }

    void actionTriggered(bool b) override;

    SelectionContextOperation m_action;
    QByteArray m_id;
};

class QMLDESIGNERCOMPONENTS_EXPORT ActionGroup : public AbstractActionGroup
{
public:
    ActionGroup(const QString &displayName,
                const QByteArray &menuId,
                const QIcon &icon,
                int priority,
                SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                SelectionContextPredicate visibility = &SelectionContextFunctors::always);

    bool isVisible(const SelectionContext &m_selectionState) const override { return m_visibility(m_selectionState); }
    bool isEnabled(const SelectionContext &m_selectionState) const override { return m_enabled(m_selectionState); }
    QByteArray category() const override { return m_category; }
    QByteArray menuId() const override { return m_menuId; }
    int priority() const override { return m_priority; }
    Type type() const override { return ContextMenu; }

    void setCategory(const QByteArray &catageoryId)
    {
        m_category = catageoryId;
    }

private:
    const QByteArray m_menuId;
    const int m_priority;
    SelectionContextPredicate m_enabled;
    SelectionContextPredicate m_visibility;
    QByteArray m_category;
};

class QMLDESIGNERCOMPONENTS_EXPORT SeparatorDesignerAction : public AbstractAction
{
public:
    SeparatorDesignerAction(const QByteArray &category, int priority);

    bool isVisible(const SelectionContext &m_selectionState) const override { return m_visibility(m_selectionState); }
    bool isEnabled(const SelectionContext &) const override { return true; }
    QByteArray category() const override { return m_category; }
    QByteArray menuId() const override { return QByteArray(); }
    int priority() const override { return m_priority; }
    Type type() const override { return ContextMenuAction; }
    void currentContextChanged(const SelectionContext &) override {}

private:
    const QByteArray m_category;
    const int m_priority;
    SelectionContextPredicate m_visibility;
};

class CommandAction : public ActionInterface
{
public:
    CommandAction(Core::Command *command,
                  const QByteArray &category,
                  int priority,
                  const QIcon &overrideIcon);

    QAction *action() const override { return m_action; }
    QByteArray category() const override  { return m_category; }
    QByteArray menuId() const override  { return QByteArray(); }
    int priority() const override { return m_priority; }
    Type type() const override { return ContextMenuAction; }
    void currentContextChanged(const SelectionContext &) override {}

private:
    QAction *m_action;
    const QByteArray m_category;
    const int m_priority;
};

class ModelNodeContextMenuAction : public AbstractAction
{
public:
    ModelNodeContextMenuAction(const QByteArray &id,
                               const QString &description,
                               const QIcon &icon,
                               const QByteArray &category,
                               const QKeySequence &key,
                               int priority,
                               SelectionContextOperation selectionAction,
                               SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                               SelectionContextPredicate visibility = &SelectionContextFunctors::always);

    bool isVisible(const SelectionContext &selectionState) const override { return m_visibility(selectionState); }
    bool isEnabled(const SelectionContext &selectionState) const override { return m_enabled(selectionState); }
    QByteArray category() const override { return m_category; }
    QByteArray menuId() const override { return m_id; }
    int priority() const override { return m_priority; }
    Type type() const override { return ContextMenuAction; }

private:
    const QByteArray m_id;
    const QByteArray m_category;
    const int m_priority;
    const SelectionContextPredicate m_enabled;
    const SelectionContextPredicate m_visibility;
};

class QMLDESIGNERCOMPONENTS_EXPORT ModelNodeAction : public ModelNodeContextMenuAction
{
public:
    ModelNodeAction(const QByteArray &id,
                    const QString &description,
                    const QIcon &icon,
                    const QString &tooltip,
                    const QByteArray &category,
                    const QKeySequence &key,
                    int priority,
                    SelectionContextOperation selectionAction,
                    SelectionContextPredicate enabled = &SelectionContextFunctors::always);

    Type type() const override { return Action; }
};

class ModelNodeFormEditorAction : public ModelNodeContextMenuAction
{
public:
    ModelNodeFormEditorAction(const QByteArray &id,
                              const QString &description,
                              const QIcon &icon,
                              const QString &tooltip,
                              const QByteArray &category,
                              const QKeySequence &key,
                              int priority,
                              SelectionContextOperation selectionAction,
                              SelectionContextPredicate enabled = &SelectionContextFunctors::always,
                              SelectionContextPredicate visible = &SelectionContextFunctors::always);

    Type type() const override { return FormEditorAction; }
};


} //QmlDesigner
