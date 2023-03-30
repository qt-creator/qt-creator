// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnodeoperations.h"
#include "abstractaction.h"
#include "bindingproperty.h"
#include "abstractactiongroup.h"
#include "qmlitemnode.h"
#include <qmldesignerplugin.h>
#include <nodemetainfo.h>

#include <coreplugin/actionmanager/command.h>

#include <utils/proxyaction.h>

#include <QAction>
#include <QMenu>

#include <functional>

namespace QmlDesigner {

using SelectionContextPredicate = std::function<bool (const SelectionContext&)>;
using SelectionContextOperation = std::function<void (const SelectionContext&)>;

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

inline bool addMouseAreaFillCheck(const SelectionContext &selectionContext)
{
    if (selectionContext.isValid() && selectionContext.singleNodeIsSelected()) {
        ModelNode node = selectionContext.currentSingleSelectedNode();
        if (node.hasMetaInfo()) {
            NodeMetaInfo nodeInfo = node.metaInfo();
            return nodeInfo.isSuitableForMouseAreaFill();
        }
    }
    return false;
}

inline bool isModel(const SelectionContext &selectionState)
{
    ModelNode node = selectionState.currentSingleSelectedNode();
    return node.metaInfo().isQtQuick3DModel();
}

inline bool modelHasMaterial(const SelectionContext &selectionState)
{
    ModelNode node = selectionState.currentSingleSelectedNode();

    BindingProperty prop = node.bindingProperty("materials");

    return prop.exists() && (!prop.expression().isEmpty() || !prop.resolveToModelNodeList().empty());
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
    for (const ModelNode &modelNode : selectionState.selectedModelNodes())
        if (modelNode.hasProperty(PropertyName(property)))
            return true;
    return false;
}

inline bool selectionHasSlot(const SelectionContext &selectionState, const char *property)
{
    for (const ModelNode &modelNode : selectionState.selectedModelNodes()) {
        for (const PropertyName &slotName : modelNode.metaInfo().slotNames()) {
            if (slotName == property)
                return true;
        }
    }

    return false;
}

inline bool singleSelectedItem(const SelectionContext &selectionState)
{
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    return itemNode.isValid();
}

bool selectionHasSameParent(const SelectionContext &selectionState);
bool selectionIsComponent(const SelectionContext &selectionState);
bool singleSelectionItemIsAnchored(const SelectionContext &selectionState);
bool singleSelectionItemIsNotAnchored(const SelectionContext &selectionState);
bool selectionIsImported3DAsset(const SelectionContext &selectionState);

} // namespace SelectionStateFunctors

class ActionTemplate : public DefaultAction
{

public:
    ActionTemplate(const QByteArray &id, const QString &description, SelectionContextOperation action)
        : DefaultAction(description), m_action(action), m_id(id)
    { }

    void actionTriggered(bool b) override
    {
        QmlDesignerPlugin::emitUsageStatisticsContextAction(QString::fromUtf8(m_id));
        m_selectionContext.setToggled(b);
        m_action(m_selectionContext);
    }

    SelectionContextOperation m_action;
    QByteArray m_id;
};

class ActionGroup : public AbstractActionGroup
{
public:
    ActionGroup(const QString &displayName, const QByteArray &menuId, const QIcon &icon, int priority,
            SelectionContextPredicate enabled = &SelectionContextFunctors::always,
            SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        AbstractActionGroup(displayName),
        m_menuId(menuId),
        m_priority(priority),
        m_enabled(enabled),
        m_visibility(visibility)
    {
        menu()->setIcon(icon);
    }

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

class SeparatorDesignerAction : public AbstractAction
{
public:
    SeparatorDesignerAction(const QByteArray &category, int priority)
        : AbstractAction()
        , m_category(category)
        , m_priority(priority)
        , m_visibility(&SelectionContextFunctors::always)
    {
        action()->setSeparator(true);
        action()->setIcon({});
    }

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
    CommandAction(Core::Command *command,  const QByteArray &category, int priority, const QIcon &overrideIcon) :
        m_action(overrideIcon.isNull() ? command->action() : Utils::ProxyAction::proxyActionWithIcon(command->action(), overrideIcon)),
        m_category(category),
        m_priority(priority)
    {}

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
    ModelNodeContextMenuAction(const QByteArray &id, const QString &description, const QIcon &icon, const QByteArray &category, const QKeySequence &key, int priority,
            SelectionContextOperation selectionAction,
            SelectionContextPredicate enabled = &SelectionContextFunctors::always,
            SelectionContextPredicate visibility = &SelectionContextFunctors::always) :
        AbstractAction(new ActionTemplate(id, description, selectionAction)),
        m_id(id),
        m_category(category),
        m_priority(priority),
        m_enabled(enabled),
        m_visibility(visibility)
    {
        action()->setShortcut(key);
        action()->setIcon(icon);
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
        ModelNodeContextMenuAction(id, description, icon, category, key, priority, selectionAction, enabled, &SelectionContextFunctors::always)
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
        ModelNodeContextMenuAction(id, description, icon, category, key, priority, selectionAction, enabled, visible)
    {
        action()->setIcon(icon);
        action()->setToolTip(tooltip);
    }


    Type type() const override { return FormEditorAction; }
};


} //QmlDesigner
