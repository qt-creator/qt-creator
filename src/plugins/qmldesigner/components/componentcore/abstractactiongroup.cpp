/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "abstractactiongroup.h"

#include <QMenu>

namespace QmlDesigner {

AbstractActionGroup::AbstractActionGroup(const QString &displayName) :
    m_displayName(displayName),
    m_menu(new QMenu)
{
    m_menu->setTitle(displayName);
    m_action = m_menu->menuAction();
}

AbstractDesignerAction::Type AbstractActionGroup::type() const
{
    return AbstractDesignerAction::Menu;
}

QAction *AbstractActionGroup::action() const
{
    return m_action;
}

QMenu *AbstractActionGroup::menu() const
{
    return m_menu.data();
}

SelectionContext AbstractActionGroup::selectionContext() const
{
    return m_selectionContext;
}

void AbstractActionGroup::currentContextChanged(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
    updateContext();
}

void AbstractActionGroup::updateContext()
{
    if (m_selectionContext.isValid()) {
        m_action->setEnabled(isEnabled(m_selectionContext));
        m_action->setVisible(isVisible(m_selectionContext));
    }
}

} // namespace QmlDesigner
