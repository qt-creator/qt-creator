/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "modelnodecontextmenu.h"
#include "designeractionmanager.h"

#include <QAction>
#include <QMenu>

namespace QmlDesigner {

namespace SelectionContextFunctors {

struct Always {
    bool operator() (const SelectionContext &) {
        return true;
    }
};

struct InBaseState {
    bool operator() (const SelectionContext &selectionState) {
        return selectionState.isInBaseState();
    }
};

struct SingleSelection {
    bool operator() (const SelectionContext &selectionState) {
        return selectionState.singleSelected();
    }
};

struct SelectionEnabled {
    bool operator() (const SelectionContext &selectionState) {
        return selectionState.showSelectionTools();
    }
};

struct SelectionNotEmpty {
    bool operator() (const SelectionContext &selectionState) {
        return !selectionState.selectedModelNodes().isEmpty();
    }
};

struct SingleSelectionNotRoot {
    bool operator() (const SelectionContext &selectionState) {
        return selectionState.singleSelected()
                && !selectionState.currentSingleSelectedNode().isRootNode();
    }
};

template <class T1, class T2>
struct And {
    bool operator() (const SelectionContext &selectionState) {
        T1 t1;
        T2 t2;
        return t1(selectionState) && t2(selectionState);
    }
};

template <class T1, class T2>
struct Or {
    bool operator() (const SelectionContext &selectionState) {
        T1 t1;
        T2 t2;
        return t1(selectionState) || t2(selectionState);
    }
};

template <class T1>
struct Not {
    bool operator() (const SelectionContext &selectionState) {
        T1 t1;
        return !t1(selectionState);
    }
};

template <char* PROPERTYNAME>
struct SelectionHasProperty {
    bool operator() (const SelectionContext &selectionState) {
        foreach (const ModelNode &modelNode, selectionState.selectedModelNodes())
            if (modelNode.hasProperty(QLatin1String(PROPERTYNAME)))
                return true;
        return false;
    }
};

struct SelectionHasSameParent {
    bool operator() (const SelectionContext &selectionState);
};

struct SelectionIsComponent {
    bool operator() (const SelectionContext &selectionState);
};

struct SingleSelectionItemIsAnchored {
    bool operator() (const SelectionContext &selectionState);
};

struct SingleSelectionItemNotAnchored {
    bool operator() (const SelectionContext &selectionState);
};

struct SingleSelectedItem {
    bool operator() (const SelectionContext &selectionState) {
        QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
        return itemNode.isValid();
    }
};

} //SelectionStateFunctors


class ComponentUtils {
public:
    static void goIntoComponent(const ModelNode &modelNode);
};

class DefaultAction : public QAction {

    Q_OBJECT

public:
    DefaultAction(const QString &description) : QAction(description, 0)
    {
        connect(this, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
    }

public slots: //virtual method instead of slot
    virtual void actionTriggered(bool )
    { }

    void setSelectionContext(const SelectionContext &selectionContext)
    {
        m_selectionContext = selectionContext;
    }

protected:
    SelectionContext m_selectionContext;
};

class DefaultDesignerAction : public AbstractDesignerAction
{
public:
    DefaultDesignerAction(const QString &description) : m_action(new DefaultAction(description))
    {}

    DefaultDesignerAction(DefaultAction *action) : m_action(action)
    {}

    virtual QAction *action() const
    { return m_action; }

    virtual void setCurrentContext(const SelectionContext &selectionContext)
    {
        m_selectionContext = selectionContext;
        updateContext();
    }

    virtual void updateContext()
    {
        m_action->setSelectionContext(m_selectionContext);
        if (m_selectionContext.isValid()) {
            m_action->setEnabled(isEnabled(m_selectionContext));
            m_action->setVisible(isVisible(m_selectionContext));
        }
    }

protected:
    DefaultAction *m_action;
    SelectionContext m_selectionContext;
};

template <class ENABLED = SelectionContextFunctors::Always,
          class VISIBILITY = SelectionContextFunctors::Always>
class MenuDesignerAction : public AbstractDesignerAction
{
public:
    MenuDesignerAction(const QString &displayName, const QString &menuId, int priority) :
        m_displayName(displayName),
        m_menuId(menuId),
        m_priority(priority),
        m_menu(new QMenu)
    {
        m_menu->setTitle(displayName);
        m_action = m_menu->menuAction();
    }

    virtual bool isVisible(const SelectionContext &m_selectionState) const
    { VISIBILITY visibility; return visibility(m_selectionState); }

    virtual bool isEnabled(const SelectionContext &m_selectionState) const
    { ENABLED enabled; return enabled(m_selectionState); }

    virtual QString category() const
    { return QString(""); }

    virtual QString menuId() const
    { return m_menuId; }

    virtual int priority() const
    { return m_priority; }

    virtual AbstractDesignerAction::Type type() const
    { return AbstractDesignerAction::Menu; }

    virtual QAction *action() const
    { return m_action; }

    virtual void setCurrentContext(const SelectionContext &selectionContext)
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
    const QString m_menuId;
    const int m_priority;
    SelectionContext m_selectionContext;
    QScopedPointer<QMenu> m_menu;
    QAction *m_action;
};

template <class VISIBILITY = SelectionContextFunctors::Always>
class SeperatorDesignerAction : public DefaultDesignerAction
{
public:
    SeperatorDesignerAction(const QString &category, int priority) :
        DefaultDesignerAction(QString()),
        m_category(category), m_priority(priority)
    { m_action->setSeparator(true); }

    virtual bool isVisible(const SelectionContext &m_selectionState) const
    { VISIBILITY visibility; return visibility(m_selectionState); }

    virtual bool isEnabled(const SelectionContext &) const
    { return true; }

    virtual QString category() const
    { return m_category; }

    virtual QString menuId() const
    { return QString(); }

    virtual int priority() const
    { return m_priority; }

    virtual Type type() const
    { return Action; }

    virtual void setCurrentContext(const SelectionContext &)
    {}
private:
    const QString m_category;
    const int m_priority;
};

template <class ACTION>
class ActionTemplate : public DefaultAction {

public:
    ActionTemplate(const QString &description) : DefaultAction(description)
    { }

public /*slots*/:
    virtual void actionTriggered(bool b)
    {
        m_selectionContext.setToggled(b);
        ACTION action;
        return action(m_selectionContext);
    }
};

template <class ACTION,
          class ENABLED = SelectionContextFunctors::Always,
          class VISIBILITY = SelectionContextFunctors::Always>
class ModelNodeActionFactory : public DefaultDesignerAction
{
public:
    ModelNodeActionFactory(const QString &description,  const QString &category, int priority) :
        DefaultDesignerAction(new ActionTemplate<ACTION>(description)),
        m_category(category),
        m_priority(priority)
    {}

    virtual bool isVisible(const SelectionContext &selectionState) const
    { VISIBILITY visibility; return visibility(selectionState); }

    virtual bool isEnabled(const SelectionContext &selectionState) const
    { ENABLED enabled; return enabled(selectionState); }

    virtual QString category() const
    { return m_category; }

    virtual QString menuId() const
    { return QString(); }

    virtual int priority() const
    { return m_priority; }

    virtual Type type() const
    { return Action; }

private:
 const QString m_category;
 const int m_priority;
};


} //QmlDesigner

#endif // MODELNODECONTEXTMENU_HELPER_H
