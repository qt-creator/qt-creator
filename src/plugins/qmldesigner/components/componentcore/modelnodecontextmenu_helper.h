/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MODELNODECONTEXTMENU_HELPER_H
#define MODELNODECONTEXTMENU_HELPER_H

#include "modelnodeoperations.h"
#include "defaultdesigneraction.h"

#include <QAction>
#include <QMenu>

namespace QmlDesigner {

typedef  bool (*SelectionContextFunction)(const SelectionContext &);

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


namespace ComponentUtils {
    void goIntoComponent(const ModelNode &modelNode);
}

class ActionTemplate : public DefaultAction
{

public:
    ActionTemplate(const QString &description, ModelNodeOperations::SelectionAction action)
        : DefaultAction(description), m_action(action)
    { }

public /*slots*/:
    virtual void actionTriggered(bool b)
    {
        m_selectionContext.setToggled(b);
        return m_action(m_selectionContext);
    }
    ModelNodeOperations::SelectionAction m_action;
};

class MenuDesignerAction : public AbstractDesignerAction
{
public:
    MenuDesignerAction(const QString &displayName, const QByteArray &menuId, int priority,
            SelectionContextFunction enabled = &SelectionContextFunctors::always,
            SelectionContextFunction visibility = &SelectionContextFunctors::always) :
        m_displayName(displayName),
        m_menuId(menuId),
        m_priority(priority),
        m_menu(new QMenu),
        m_enabled(enabled),
        m_visibility(visibility)
    {
        m_menu->setTitle(displayName);
        m_action = m_menu->menuAction();
    }

    bool isVisible(const SelectionContext &m_selectionState) const { return m_visibility(m_selectionState); }
    bool isEnabled(const SelectionContext &m_selectionState) const { return m_enabled(m_selectionState); }
    QByteArray category() const { return QByteArray(); }
    QByteArray menuId() const { return m_menuId; }
    int priority() const { return m_priority; }
    AbstractDesignerAction::Type type() const { return AbstractDesignerAction::Menu; }
    QAction *action() const { return m_action; }

    virtual void currentContextChanged(const SelectionContext &selectionContext)
    {
        m_selectionContext = selectionContext;
        updateContext();
    }

    virtual void updateContext()
    {
        if (m_selectionContext.isValid()) {
            m_action->setEnabled(isEnabled(m_selectionContext));
            m_action->setVisible(isVisible(m_selectionContext));
        }
    }

protected:
    const QString m_displayName;
    const QByteArray m_menuId;
    const int m_priority;
    SelectionContext m_selectionContext;
    QScopedPointer<QMenu> m_menu;
    QAction *m_action;
    SelectionContextFunction m_enabled;
    SelectionContextFunction m_visibility;
};

class SeperatorDesignerAction : public DefaultDesignerAction
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
    Type type() const { return Action; }
    void currentContextChanged(const SelectionContext &) {}

private:
    const QByteArray m_category;
    const int m_priority;
    SelectionContextFunction m_visibility;
};

class ModelNodeAction : public DefaultDesignerAction
{
public:
    ModelNodeAction(const QString &description,  const QByteArray &category, int priority,
            ModelNodeOperations::SelectionAction selectionAction,
            SelectionContextFunction enabled = &SelectionContextFunctors::always,
            SelectionContextFunction visibility = &SelectionContextFunctors::always) :
        DefaultDesignerAction(new ActionTemplate(description, selectionAction)),
        m_category(category),
        m_priority(priority),
        m_enabled(enabled),
        m_visibility(visibility)
    {}

    bool isVisible(const SelectionContext &selectionState) const { return m_visibility(selectionState); }
    bool isEnabled(const SelectionContext &selectionState) const { return m_enabled(selectionState); }
    QByteArray category() const { return m_category; }
    QByteArray menuId() const { return QByteArray(); }
    int priority() const { return m_priority; }
    Type type() const { return Action; }

private:
    const QByteArray m_category;
    const int m_priority;
    const SelectionContextFunction m_enabled;
    const SelectionContextFunction m_visibility;
};


} //QmlDesigner

#endif // MODELNODECONTEXTMENU_HELPER_H
