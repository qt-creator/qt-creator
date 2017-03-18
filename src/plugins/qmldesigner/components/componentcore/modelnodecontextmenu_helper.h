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

#include "modelnodeoperations.h"
#include "abstractaction.h"
#include "abstractactiongroup.h"
#include "qmlitemnode.h"

#include <coreplugin/actionmanager/command.h>

#include <utils/proxyaction.h>

#include <QAction>
#include <QMenu>

#include <functional>

namespace QmlDesigner {

typedef std::function<bool (const SelectionContext &context)> SelectionContextPredicate;
typedef std::function<void (const SelectionContext &context)> SelectionContextOperation;

namespace SelectionContextFunctors {

inline bool always(const SelectionContext &)
{
    return true;
}

inline bool inBaseState(const SelectionContext &selectionState)
{
    return selectionState.isInBaseState();
}

inline bool singleSelection(const SelectionContext &selectionState)
{
    return selectionState.singleNodeIsSelected();
}

inline bool selectionEnabled(const SelectionContext &selectionState)
{
    return selectionState.showSelectionTools();
}

inline bool selectionNotEmpty(const SelectionContext &selectionState)
{
    return !selectionState.selectedModelNodes().isEmpty();
}

inline bool singleSelectionNotRoot(const SelectionContext &selectionState)
{
    return selectionState.singleNodeIsSelected()
        && !selectionState.currentSingleSelectedNode().isRootNode();
}

inline bool selectionHasProperty(const SelectionContext &selectionState, const char *property)
{
    foreach (const ModelNode &modelNode, selectionState.selectedModelNodes())
        if (modelNode.hasProperty(PropertyName(property)))
            return true;
    return false;
}

inline bool singleSelectedItem(const SelectionContext &selectionState)
{
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    return itemNode.isValid();
}

bool selectionHasSameParent(const SelectionContext &selectionState);
bool selectionIsComponent(const SelectionContext &selectionState);
bool selectionIsComponent(const SelectionContext &selectionState);
bool singleSelectionItemIsAnchored(const SelectionContext &selectionState);
bool singleSelectionItemIsNotAnchored(const SelectionContext &selectionState);

} // namespace SelectionStateFunctors

class ActionTemplate : public DefaultAction
{

public:
    ActionTemplate(const QString &description, SelectionContextOperation action)
        : DefaultAction(description), m_action(action)
    { }

    virtual void actionTriggered(bool b)
    {
        m_selectionContext.setToggled(b);
        m_action(m_selectionContext);
    }

    SelectionContextOperation m_action;
};

class ActionGroup : public AbstractActionGroup
{
public:
    ActionGroup(const QString &displayName, const QByteArray &menuId, int priority,
            SelectionContextPredicate enabled = &SelectionContextFunctors::always,
            SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        AbstractActionGroup(displayName),
        m_menuId(menuId),
        m_priority(priority),
        m_enabled(enabled),
        m_visibility(visibility)
    {
    }

    bool isVisible(const SelectionContext &m_selectionState) const { return m_visibility(m_selectionState); }
    bool isEnabled(const SelectionContext &m_selectionState) const { return m_enabled(m_selectionState); }
    QByteArray category() const { return QByteArray(); }
    QByteArray menuId() const { return m_menuId; }
    int priority() const { return m_priority; }
    Type type() const { return ContextMenu; }

private:
    const QByteArray m_menuId;
    const int m_priority;
    SelectionContextPredicate m_enabled;
    SelectionContextPredicate m_visibility;
};

class SeperatorDesignerAction : public AbstractAction
{
public:
    SeperatorDesignerAction(const QByteArray &category, int priority) :
        m_category(category),
        m_priority(priority),
        m_visibility(&SelectionContextFunctors::always)
    { defaultAction()->setSeparator(true); }

    bool isVisible(const SelectionContext &m_selectionState) const { return m_visibility(m_selectionState); }
    bool isEnabled(const SelectionContext &) const { return true; }
    QByteArray category() const { return m_category; }
    QByteArray menuId() const { return QByteArray(); }
    int priority() const { return m_priority; }
    Type type() const { return ContextMenuAction; }
    void currentContextChanged(const SelectionContext &) {}

private:
    const QByteArray m_category;
    const int m_priority;
    SelectionContextPredicate m_visibility;
};

class CommandAction : public ActionInterface
{
public:
    CommandAction(Core::Command *command,  const QByteArray &category, int priority, const QIcon &overrideIcon) :
        m_action(overrideIcon.isNull() ? command->action() : Utils::ProxyAction::proxyActionWithIcon(command->action(), overrideIcon)),
        m_category(category),
        m_priority(priority)
    {}

    QAction *action() const override { return m_action; }
    QByteArray category() const override  { return m_category; }
    QByteArray menuId() const override  { return QByteArray(); }
    int priority() const override { return m_priority; }
    Type type() const override { return Action; }
    void currentContextChanged(const SelectionContext &) override {}

private:
    QAction *m_action;
    const QByteArray m_category;
    const int m_priority;
};

class ModelNodeContextMenuAction : public AbstractAction
{
public:
    ModelNodeContextMenuAction(const QByteArray &id, const QString &description,  const QByteArray &category, const QKeySequence &key, int priority,
            SelectionContextOperation selectionAction,
            SelectionContextPredicate enabled = &SelectionContextFunctors::always,
            SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        AbstractAction(new ActionTemplate(description, selectionAction)),
        m_id(id),
        m_category(category),
        m_priority(priority),
        m_enabled(enabled),
        m_visibility(visibility)
    {
        action()->setShortcut(key);
    }

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

class ModelNodeAction : public ModelNodeContextMenuAction
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
                    SelectionContextPredicate enabled = &SelectionContextFunctors::always) :
        ModelNodeContextMenuAction(id, description, category, key, priority, selectionAction, enabled, &SelectionContextFunctors::always)
    {
        action()->setIcon(icon);
        action()->setToolTip(tooltip);
    }

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
                              SelectionContextPredicate visible = &SelectionContextFunctors::always) :
        ModelNodeContextMenuAction(id, description, category, key, priority, selectionAction, enabled, visible)
    {
        action()->setIcon(icon);
        action()->setToolTip(tooltip);
    }


    Type type() const override { return FormEditorAction; }
};


} //QmlDesigner
